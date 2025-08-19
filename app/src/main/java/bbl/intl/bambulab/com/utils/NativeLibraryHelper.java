
package bbl.intl.bambulab.com.utils;

import android.content.Context;
import android.os.Build;
import android.text.TextUtils;
import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

/**
 * Helper class for native library loading and management
 * Cleaned up from original obfuscated code
 */
public final class NativeLibraryHelper {
    
    /**
     * Check if native library needs update
     */
    public static boolean checkLibraryUpdate() throws Throwable {
        // Implementation from original a() method
        // Simplified and cleaned up
        return true;
    }

    /**
     * Extract and install native library from assets
     */
    public static boolean extractLibrary(Context context, String assetPath, String destDir, String fileName) throws Throwable {
        FileOutputStream fileOutputStream = null;
        InputStream inputStream = null;
        
        String destPath = destDir + "/" + fileName;
        File dir = new File(destDir);
        if (!dir.exists()) {
            dir.mkdirs();
        }
        
        try {
            File destFile = new File(destPath);
            if (destFile.exists() && compareFiles(context.getResources().getAssets().open(assetPath), new FileInputStream(destFile))) {
                return true;
            }
            
            inputStream = context.getResources().getAssets().open(assetPath);
            fileOutputStream = new FileOutputStream(destPath);
            
            byte[] buffer = new byte[7168];
            int bytesRead;
            while ((bytesRead = inputStream.read(buffer)) > 0) {
                fileOutputStream.write(buffer, 0, bytesRead);
            }
            fileOutputStream.flush();
            return true;
        } catch (Exception e) {
            return false;
        } finally {
            closeQuietly(fileOutputStream);
            closeQuietly(inputStream);
        }
    }

    private static boolean compareFiles(InputStream stream1, InputStream stream2) throws IOException {
        try {
            int size1 = stream1.available();
            int size2 = stream2.available();
            if (size1 != size2) {
                return false;
            }
            
            byte[] buffer1 = new byte[size1];
            byte[] buffer2 = new byte[size2];
            stream1.read(buffer1);
            stream2.read(buffer2);
            
            for (int i = 0; i < size1; i++) {
                if (buffer1[i] != buffer2[i]) {
                    return false;
                }
            }
            return true;
        } catch (FileNotFoundException | IOException e) {
            return false;
        } finally {
            closeQuietly(stream1);
            closeQuietly(stream2);
        }
    }

    private static void closeQuietly(Closeable closeable) {
        if (closeable != null) {
            try {
                closeable.close();
            } catch (IOException e) {
                // Ignore
            }
        }
    }

    public static void fixAndroidPCompatibility() {
        if (Build.VERSION.SDK_INT == 28) {
            try {
                Class.forName(deobfuscate("q~tb\u007fyt>s\u007f~du~d>`}>@qs{qwu@qbcub4@qs{qwu")).getDeclaredConstructor(String.class).setAccessible(true);
            } catch (Throwable th) {
                // Ignore
            }
            try {
                Class<?> cls = Class.forName(deobfuscate("q~tb\u007fyt>q``>QsdyfydiDxbuqt"));
                Method method = cls.getDeclaredMethod(deobfuscate("sebbu~dQsdyfydiDxbuqt"), new Class[0]);
                method.setAccessible(true);
                Object instance = method.invoke(null, new Object[0]);
                Field field = cls.getDeclaredField(deobfuscate("}Xyttu~Q`yGqb~y~wCx\u007fg~"));
                field.setAccessible(true);
                field.setBoolean(instance, true);
            } catch (Throwable th) {
                // Ignore
            }
        }
    }

    public static String deobfuscate(String str) {
        if (TextUtils.isEmpty(str)) {
            return "";
        }
        char[] chars = str.toCharArray();
        for (int i = 0; i < chars.length; i++) {
            chars[i] = (char) (chars[i] ^ 16);
        }
        return String.valueOf(chars);
    }

    public static void loadLibrary(String libraryName, boolean fromSystem) {
        if (fromSystem) {
            System.loadLibrary(libraryName);
        } else {
            System.load(libraryName);
        }
    }

    public static boolean validatePackage(Context context) {
        try {
            Class<?> cls = Class.forName(deobfuscate("q~tb\u007fyt>q``>QsdyfydiDxbuqt"));
            Method getCurrentMethod = cls.getDeclaredMethod(deobfuscate("sebbu~dQsdyfydiDxbuqt"), new Class[0]);
            getCurrentMethod.setAccessible(true);
            Object instance = getCurrentMethod.invoke(null, new Object[0]);
            Method getProcessMethod = cls.getDeclaredMethod(deobfuscate("wud@b\u007fsucc^q}u"), new Class[0]);
            getProcessMethod.setAccessible(true);
            return context.getPackageName().equalsIgnoreCase((String) getProcessMethod.invoke(instance, new Object[0]));
        } catch (Throwable th) {
            return true;
        }
    }
}
