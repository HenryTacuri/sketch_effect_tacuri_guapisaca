package vision.applicacionnativa;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.View;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Environment;
import android.widget.Button;
import android.widget.ImageView;
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;


import vision.applicacionnativa.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    private Button connectButton;
    private ImageView videoStreaming;

    private String stream_url = "http://192.168.18.182:81/stream";
    private HttpURLConnection connection;
    private boolean connected = false;


    static {
        System.loadLibrary("applicacionnativa");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        ActivityMainBinding binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        connectButton = findViewById(R.id.button);
        videoStreaming = findViewById(R.id.imageView);

        connectButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (connected) {
                    disconnect();
                } else {
                    connect();
                }
            }
        });
    }

    private void connect() {
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                BufferedInputStream bis = null;
                try {
                    URL url = new URL(stream_url);
                    connection = (HttpURLConnection) url.openConnection();
                    connection.setRequestMethod("GET");
                    connection.setConnectTimeout(1000 * 5);
                    connection.setReadTimeout(1000 * 5);
                    connection.setDoInput(true);
                    connection.connect();

                    if (connection.getResponseCode() == 200) {
                        connected = true;
                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                connectButton.setText("Desconectar");
                            }
                        });

                        InputStream in = connection.getInputStream();
                        InputStreamReader  isr = new InputStreamReader(in);
                        BufferedReader br = new BufferedReader(isr);

                        String data;
                        int len;
                        byte[] buffer;

                        while ((data = br.readLine()) != null) {
                            if (data.contains("Content-Type:")) {

                                data = br.readLine();
                                len = Integer.parseInt(data.split(":")[1].trim());
                                bis = new BufferedInputStream(in);
                                buffer = new byte[len];

                                int t = 0;
                                while (t < len) {
                                    t += bis.read(buffer, t, len - t);
                                }


                                Bytes2ImageFile(buffer, getExternalFilesDir(Environment.DIRECTORY_PICTURES) + "/0A.jpg");
                                final Bitmap bitmap = BitmapFactory.decodeFile(getExternalFilesDir(Environment.DIRECTORY_PICTURES) + "/0A.jpg");


                                android.graphics.Bitmap bOut = bitmap.copy(bitmap.getConfig(), true);
                                filtroSketch(bitmap, bOut);
                                runOnUiThread(new Runnable() {
                                    @Override
                                    public void run() {
                                        videoStreaming.setImageBitmap(bOut);
                                    }
                                });
                            }
                        }
                    }
                } catch (MalformedURLException e) {
                    e.printStackTrace();
                } catch (IOException e) {
                    e.printStackTrace();
                } finally {
                    disconnect();
                }
            }
        });
        thread.start();
    }


    private void disconnect() {
        if (connection != null) {
            connection.disconnect();
        }

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                connectButton.setText("Conectar");
            }
        });

        connected = false;
    }


    private void Bytes2ImageFile(byte[] bytes, String fileName) {
        try {
            File file = new File(fileName);
            FileOutputStream fos = new FileOutputStream(file);
            fos.write(bytes, 0, bytes.length);
            fos.flush();
            fos.close();

        } catch (Exception e)
        {
            e.printStackTrace();
        }
    }


    public native void filtroSketch(android.graphics.Bitmap in, android.graphics.Bitmap out);
}

