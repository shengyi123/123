package com.example.user.htsoboe;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.util.Timer;

public class MainActivity extends AppCompatActivity {

    private Timer mLatencyUpdater;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    public native int HtsEngine(int argc ,char argv);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        Button button =findViewById(R.id.button);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                HtsEngine(46,'1');
            }
        });
        // initialize native audio system
        PlaybackEngine.create(this);
    }

    @Override
    protected void onDestroy() {
        if (mLatencyUpdater != null) mLatencyUpdater.cancel();
        PlaybackEngine.delete();
        super.onDestroy();
    }


}
