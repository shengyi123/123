package com.shengyi.jniadd;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import android.widget.Button;
import android.widget.EditText;

public class MainActivity extends AppCompatActivity {




    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final TextView tv = findViewById(R.id.sample_text);
        Button button1 = findViewById(R.id.button);



        button1.setOnClickListener(new Button.OnClickListener(){
            @Override
            public void onClick(View v) {
                //TODO Auto-generated method stub
                tv.setText("結果:"+ new  JNIAdd().Add(10,5));
            }
        });


    }

}
