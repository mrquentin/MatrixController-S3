package com.mrquentinet.matrixcontroller;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Insets;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.text.InputType;
import android.view.KeyEvent;
import android.view.WindowInsets;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.window.OnBackInvokedCallback;
import android.window.OnBackInvokedDispatcher;

/**
 * The controller's web UI in a full-screen WebView. Chrome only installs real apps for HTTPS sites
 * and the board serves plain HTTP, so this stands in for the home-screen install.
 */
public class MainActivity extends Activity {
    private static final String DEFAULT_URL = "http://matrixcontroller.local/";
    private static final String ACTION_CHANGE_ADDRESS = "com.mrquentinet.matrixcontroller.CHANGE_ADDRESS";
    private static final int PICK_FILE = 1;

    private SharedPreferences prefs;
    private WebView web;
    private ValueCallback<Uri[]> pendingFileCallback;
    private AlertDialog dialog;

    // Android 13+ back gesture: registered only while the page has history, so the last back
    // closes the app as usual
    private OnBackInvokedCallback backCallback;
    private boolean backRegistered;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        prefs = getPreferences(MODE_PRIVATE);

        web = new WebView(this);
        web.setBackgroundColor(getColor(R.color.page));
        WebSettings settings = web.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        web.setWebViewClient(new Client());
        web.setWebChromeClient(new ChromeClient());

        FrameLayout root = new FrameLayout(this);
        root.addView(web);
        if (Build.VERSION.SDK_INT >= 30) {
            // Drawn edge to edge (forced from Android 15), so keep the page out from under the
            // status and navigation bars and the keyboard; the window background shows behind them
            getWindow().setDecorFitsSystemWindows(false);
            root.setOnApplyWindowInsetsListener((v, insets) -> {
                Insets bars = insets.getInsets(WindowInsets.Type.systemBars()
                        | WindowInsets.Type.displayCutout() | WindowInsets.Type.ime());
                v.setPadding(bars.left, bars.top, bars.right, bars.bottom);
                return WindowInsets.CONSUMED;
            });
        }
        setContentView(root);

        if (Build.VERSION.SDK_INT >= 33) {
            backCallback = () -> web.goBack();
        }

        if (savedInstanceState != null) {
            web.restoreState(savedInstanceState);
        } else if (ACTION_CHANGE_ADDRESS.equals(getIntent().getAction())) {
            askAddress(null);
        } else {
            web.loadUrl(url());
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (ACTION_CHANGE_ADDRESS.equals(intent.getAction())) askAddress(null);
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        web.saveState(outState);
    }

    @Override
    protected void onDestroy() {
        if (dialog != null) dialog.dismiss();
        web.destroy();
        super.onDestroy();
    }

    // Android 12 and older; 13+ uses backCallback
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (Build.VERSION.SDK_INT < 33 && keyCode == KeyEvent.KEYCODE_BACK && web.canGoBack()) {
            web.goBack();
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    private String url() {
        return prefs.getString("url", DEFAULT_URL);
    }

    private void updateBackCallback() {
        if (Build.VERSION.SDK_INT < 33 || web.canGoBack() == backRegistered) return;
        backRegistered = web.canGoBack();
        if (backRegistered) {
            getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
                    OnBackInvokedDispatcher.PRIORITY_DEFAULT, backCallback);
        } else {
            getOnBackInvokedDispatcher().unregisterOnBackInvokedCallback(backCallback);
        }
    }

    /** Asks for the board's address, with why it's asking (the page didn't load) or null. */
    private void askAddress(String message) {
        if (dialog != null) dialog.dismiss();

        EditText input = new EditText(this);
        input.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_URI);
        input.setSingleLine();
        input.setText(url());
        FrameLayout box = new FrameLayout(this);
        int pad = Math.round(20 * getResources().getDisplayMetrics().density);
        box.setPadding(pad, pad / 2, pad, 0);
        box.addView(input);

        AlertDialog.Builder builder = new AlertDialog.Builder(this)
                .setTitle(message == null ? R.string.address_title : R.string.unreachable_title)
                .setView(box)
                .setPositiveButton(message == null ? R.string.open : R.string.retry, (d, w) -> {
                    prefs.edit().putString("url", normalize(input.getText().toString())).apply();
                    web.loadUrl(url());
                })
                .setNegativeButton(R.string.cancel, null);
        if (message != null) builder.setMessage(message);
        dialog = builder.show();
    }

    /** "192.168.1.5" → "http://192.168.1.5/" */
    private static String normalize(String address) {
        String url = address.trim();
        if (url.isEmpty()) return DEFAULT_URL;
        if (!url.contains("://")) url = "http://" + url;
        String path = Uri.parse(url).getPath();
        if (path == null || path.isEmpty()) url += "/";
        return url;
    }

    private class Client extends WebViewClient {
        /** The board's own pages stay here; links elsewhere (GitHub releases…) open in the browser. */
        @Override
        public boolean shouldOverrideUrlLoading(WebView view, WebResourceRequest request) {
            Uri target = request.getUrl();
            String host = Uri.parse(url()).getHost();
            if (host != null && host.equalsIgnoreCase(target.getHost())) return false;
            try {
                startActivity(new Intent(Intent.ACTION_VIEW, target));
            } catch (ActivityNotFoundException ignored) {
            }
            return true;
        }

        @Override
        public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error) {
            if (!request.isForMainFrame()) return;
            view.loadUrl("about:blank");
            askAddress(getString(R.string.unreachable_message));
        }

        @Override
        public void doUpdateVisitedHistory(WebView view, String url, boolean isReload) {
            updateBackCallback();
        }
    }

    private class ChromeClient extends WebChromeClient {
        /** The firmware page's "choose firmware.bin" button. */
        @Override
        public boolean onShowFileChooser(WebView view, ValueCallback<Uri[]> callback, FileChooserParams params) {
            if (pendingFileCallback != null) pendingFileCallback.onReceiveValue(null);
            pendingFileCallback = callback;
            try {
                startActivityForResult(params.createIntent(), PICK_FILE);
            } catch (ActivityNotFoundException e) {
                pendingFileCallback = null;
                return false;
            }
            return true;
        }
    }

    @Override
    @SuppressWarnings("deprecation")
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != PICK_FILE || pendingFileCallback == null) return;
        pendingFileCallback.onReceiveValue(WebChromeClient.FileChooserParams.parseResult(resultCode, data));
        pendingFileCallback = null;
    }
}
