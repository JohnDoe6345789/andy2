
package bbl.intl.bambulab.com;

import android.app.Activity;
import android.os.Bundle;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.TextView;
import moc.balubmab.ltni.lbb.Configuration;

public class SettingsActivity extends Activity {
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        setContentView(createLayout());
        setTitle("Settings");
    }
    
    private LinearLayout createLayout() {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(40, 40, 40, 40);
        
        // Title
        TextView titleText = new TextView(this);
        titleText.setText("Bambu Lab Settings");
        titleText.setTextSize(20);
        titleText.setPadding(0, 0, 0, 30);
        layout.addView(titleText);
        
        // Crash reporting setting
        CheckBox crashReportCheckbox = new CheckBox(this);
        crashReportCheckbox.setText("Enable Crash Reporting");
        crashReportCheckbox.setChecked(Configuration.ENABLE_CRASH_REPORT);
        crashReportCheckbox.setOnCheckedChangeListener((buttonView, isChecked) -> {
            Configuration.ENABLE_CRASH_REPORT = isChecked;
        });
        layout.addView(crashReportCheckbox);
        
        // PT setting
        CheckBox ptCheckbox = new CheckBox(this);
        ptCheckbox.setText("Enable PT");
        ptCheckbox.setChecked(Configuration.ENABLE_PT);
        ptCheckbox.setOnCheckedChangeListener((buttonView, isChecked) -> {
            Configuration.ENABLE_PT = isChecked;
        });
        layout.addView(ptCheckbox);
        
        // Version info
        TextView versionText = new TextView(this);
        versionText.setText("\nApp Version: 1.0.0\nIOTC Library: Native");
        versionText.setPadding(0, 20, 0, 0);
        layout.addView(versionText);
        
        return layout;
    }
}
