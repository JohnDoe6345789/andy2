
package bbl.intl.bambulab.com;

import android.app.Activity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;
import android.util.Log;
import com.bambulab.iotc.IOTCManager;

public class CameraActivity extends Activity implements SurfaceHolder.Callback {
    private static final String TAG = "CameraActivity";
    
    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;
    private Button connectButton;
    private Button disconnectButton;
    private IOTCManager iotcManager;
    private String deviceUID;
    private int sessionId = -1;
    private boolean isStreaming = false;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Get device UID from intent
        deviceUID = getIntent().getStringExtra("device_uid");
        if (deviceUID == null) {
            Toast.makeText(this, "No device UID provided", Toast.LENGTH_SHORT).show();
            finish();
            return;
        }
        
        // Create layout programmatically
        setContentView(createLayout());
        
        // Initialize IOTC manager
        iotcManager = new IOTCManager();
        
        Log.d(TAG, "CameraActivity created for device: " + deviceUID);
    }
    
    private View createLayout() {
        // Create a simple vertical layout with surface view and buttons
        android.widget.LinearLayout layout = new android.widget.LinearLayout(this);
        layout.setOrientation(android.widget.LinearLayout.VERTICAL);
        
        // Surface view for video display
        surfaceView = new SurfaceView(this);
        surfaceView.setLayoutParams(new android.widget.LinearLayout.LayoutParams(
            android.widget.LinearLayout.LayoutParams.MATCH_PARENT, 0, 1.0f));
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
        layout.addView(surfaceView);
        
        // Button container
        android.widget.LinearLayout buttonLayout = new android.widget.LinearLayout(this);
        buttonLayout.setOrientation(android.widget.LinearLayout.HORIZONTAL);
        buttonLayout.setLayoutParams(new android.widget.LinearLayout.LayoutParams(
            android.widget.LinearLayout.LayoutParams.MATCH_PARENT, 
            android.widget.LinearLayout.LayoutParams.WRAP_CONTENT));
        
        // Connect button
        connectButton = new Button(this);
        connectButton.setText("Connect");
        connectButton.setOnClickListener(v -> connectToDevice());
        buttonLayout.addView(connectButton);
        
        // Disconnect button
        disconnectButton = new Button(this);
        disconnectButton.setText("Disconnect");
        disconnectButton.setEnabled(false);
        disconnectButton.setOnClickListener(v -> disconnectFromDevice());
        buttonLayout.addView(disconnectButton);
        
        layout.addView(buttonLayout);
        return layout;
    }
    
    private void connectToDevice() {
        new Thread(() -> {
            try {
                // Connect to device
                sessionId = iotcManager.connectByUID(deviceUID);
                
                if (sessionId >= 0) {
                    // Start video channel
                    int channelResult = iotcManager.startChannel(sessionId, (byte)0);
                    
                    runOnUiThread(() -> {
                        if (channelResult >= 0) {
                            connectButton.setEnabled(false);
                            disconnectButton.setEnabled(true);
                            isStreaming = true;
                            Toast.makeText(this, "Connected to " + deviceUID, Toast.LENGTH_SHORT).show();
                            startVideoStream();
                        } else {
                            Toast.makeText(this, "Failed to start video channel", Toast.LENGTH_SHORT).show();
                        }
                    });
                } else {
                    runOnUiThread(() -> {
                        Toast.makeText(this, "Connection failed: " + sessionId, Toast.LENGTH_SHORT).show();
                    });
                }
            } catch (Exception e) {
                runOnUiThread(() -> {
                    Toast.makeText(this, "Connection error: " + e.getMessage(), Toast.LENGTH_SHORT).show();
                });
                Log.e(TAG, "Connection error", e);
            }
        }).start();
    }
    
    private void disconnectFromDevice() {
        if (sessionId >= 0) {
            isStreaming = false;
            iotcManager.closeSession(sessionId);
            sessionId = -1;
        }
        
        connectButton.setEnabled(true);
        disconnectButton.setEnabled(false);
        Toast.makeText(this, "Disconnected", Toast.LENGTH_SHORT).show();
    }
    
    private void startVideoStream() {
        if (!isStreaming || sessionId < 0) return;
        
        new Thread(() -> {
            byte[] buffer = new byte[1024 * 1024]; // 1MB buffer
            
            while (isStreaming && sessionId >= 0) {
                try {
                    int bytesRead = iotcManager.readData(sessionId, buffer, 5000);
                    
                    if (bytesRead > 0) {
                        // Process video data here
                        // In a real implementation, you would decode H.264 and display on surface
                        Log.d(TAG, "Received " + bytesRead + " bytes of video data");
                        
                        // For now, just log that we're receiving data
                        runOnUiThread(() -> {
                            // Update UI to show streaming status
                        });
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Error reading video stream", e);
                    break;
                }
            }
            
            Log.d(TAG, "Video stream ended");
        }).start();
    }
    
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "Surface created");
    }
    
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.d(TAG, "Surface changed: " + width + "x" + height);
    }
    
    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d(TAG, "Surface destroyed");
        isStreaming = false;
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        disconnectFromDevice();
    }
}
