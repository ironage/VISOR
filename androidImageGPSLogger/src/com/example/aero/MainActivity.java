package com.example.aero;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.hardware.Camera;
import android.hardware.Camera.PictureCallback;
import android.location.Address;
import android.location.Geocoder;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.provider.MediaStore.Images.Media;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.Toast;



public class MainActivity extends Activity {
	static final int REQUEST_IMAGE_CAPTURE = 1;
	String mCurrentPhotoPath;
	static final int REQUEST_TAKE_PHOTO = 1;
	static final int TAKE_PHOTO_CODE = 0;
	private ImageView mImageView;

	private void dispatchTakePictureIntent() {
	    Intent takePictureIntent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
	    // Ensure that there's a camera activity to handle the intent
	    if (takePictureIntent.resolveActivity(getPackageManager()) != null) {
	        // Create the File where the photo should go
	        File photoFile = null;
	        try {
	            photoFile = createImageFile();
	        } catch (IOException ex) {
	            // Error occurred while creating the File
	        }
	        // Continue only if the File was successfully created
	        if (photoFile != null) {
	            takePictureIntent.putExtra(MediaStore.EXTRA_OUTPUT,
	                    Uri.fromFile(photoFile));
	            System.out.println("photo file: " + photoFile.getAbsolutePath());
	            startActivityForResult(takePictureIntent, REQUEST_TAKE_PHOTO);
	        }
	    }
	}
	private File createImageFile() throws IOException {
	    // Create an image file name
	    String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date());
	    String imageFileName = "JPEG_" + timeStamp + "_";
	    File storageDir = Environment.getExternalStoragePublicDirectory(
	            Environment.DIRECTORY_PICTURES);
	    File image = File.createTempFile(
	        imageFileName,  /* prefix */
	        ".jpg",         /* suffix */
	        storageDir      /* directory */
	    );

	    // Save a file: path for use with ACTION_VIEW intents
	    mCurrentPhotoPath = "file:" + image.getAbsolutePath();
	    return image;
	}
//	
//	private void dispatchTakePictureIntent() {
//	    Intent takePictureIntent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
//	    if (takePictureIntent.resolveActivity(getPackageManager()) != null) {
//	        startActivityForResult(takePictureIntent, REQUEST_IMAGE_CAPTURE);
//	    }
//	}
	
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
	    if (requestCode == REQUEST_IMAGE_CAPTURE && resultCode == RESULT_OK) {
	        Bundle extras = data.getExtras();
	        Bitmap imageBitmap = (Bitmap) extras.get("data");
	        mImageView.setImageBitmap(imageBitmap);
	    }

	    if (requestCode == TAKE_PHOTO_CODE && resultCode == RESULT_OK) {
	        Log.d("CameraDemo", "Pic saved");
	    }
	}	
	
    private CameraPreview mPreview;
    public static final int MEDIA_TYPE_IMAGE = 1;
    
    /** Create a File for saving an image or video */
    private static List<File> getOutputMediaFile(int type){
        // To be safe, you should check that the SDCard is mounted
        // using Environment.getExternalStorageState() before doing this.

        File mediaStorageDir = new File(Environment.getExternalStoragePublicDirectory(
                  Environment.DIRECTORY_PICTURES), "MyCameraApp");
        System.out.println("output : " + mediaStorageDir.getAbsolutePath());
        // This location works best if you want the created images to be shared
        // between applications and persist after your app has been uninstalled.

        // Create the storage directory if it does not exist
        if (! mediaStorageDir.exists()){
            if (! mediaStorageDir.mkdirs()){
                Log.d("MyCameraApp", "failed to create directory");
                return null;
            }
        }

        // Create a media file name
        String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date());
        List<File> list = new ArrayList<File>();
        
        if (type == MEDIA_TYPE_IMAGE){
            File mediaFile = new File(mediaStorageDir.getPath() + File.separator +
            "IMG_"+ timeStamp + ".jpg");
            File dataFile = new File(mediaStorageDir.getPath() + File.separator + 
            "IMG_"+ timeStamp + ".txt");
            list.add(mediaFile);
            list.add(dataFile);
        } else {
            return null;
        }

        return list;
    }

    private PictureCallback mPicture = new PictureCallback() {

        @Override
        public void onPictureTaken(byte[] data, Camera camera) {
        	String TAG = "LOGS:";
            List<File> list = getOutputMediaFile(MEDIA_TYPE_IMAGE);
            //File gpsFile = pictureFile.getPath()
            if (list == null){
                Log.d(TAG, "Error creating media file, check storage permissions: ");
                return;
            }

            try {
                FileOutputStream fos = new FileOutputStream(list.get(0));
                fos.write(data);
                fos.close();
                
                FileOutputStream fos2 = new FileOutputStream(list.get(1));
                String dataStr = String.format("%f,%f,%f", latitude, longitude, altitude);
                fos2.write(dataStr.getBytes());
                fos2.close();
            } catch (FileNotFoundException e) {
                Log.d(TAG, "File not found: " + e.getMessage());
            } catch (IOException e) {
                Log.d(TAG, "Error accessing file: " + e.getMessage());
            }
        }
    };
    private Camera mCamera;
    //private SurfaceView mPreview;

    private void releaseCamera(){
        if (mCamera != null){
            mCamera.release();        // release the camera for other applications
            mCamera = null;
        }
    }
    @Override
    protected void onPause() {
        super.onPause();
        releaseCamera();              // release the camera immediately on pause event
    }
    Timer timer;
    private TimerTask task = new TimerTask() {
		@Override
		public void run() {
            // get an image from the camera
        	if (mCamera != null) {
        		mCamera.startPreview();
        		mCamera.takePicture(null, null, mPicture);
        	}			
		}
    	
    };
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    
    LocationManager locationManager;
    /*---------- Listener class to get coordinates ------------- */
    private class MyLocationListener implements LocationListener {
    	private String TAG = "locationManager";
        @Override
        public void onLocationChanged(Location loc) {
        	latitude = loc.getLatitude();
        	longitude = loc.getLongitude();
        	altitude = loc.getAltitude();
            Toast.makeText(
                    getBaseContext(),
                    "Location changed: Lat: " + loc.getLatitude() + " Lng: "
                        + loc.getLongitude(), Toast.LENGTH_SHORT).show();
            String longitude = "Longitude: " + loc.getLongitude();
            Log.v(TAG, longitude);
            String latitude = "Latitude: " + loc.getLatitude();
            Log.v(TAG, latitude);
        }

        @Override
        public void onProviderDisabled(String provider) {}

        @Override
        public void onProviderEnabled(String provider) {}

        @Override
        public void onStatusChanged(String provider, int status, Bundle extras) {}
    }
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
		
		locationManager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
		LocationListener locationListener = new MyLocationListener();
		locationManager.requestLocationUpdates(
		LocationManager.GPS_PROVIDER, 5000, 10, locationListener);
		
		mCamera = Camera.open();
		Camera.Parameters params = mCamera.getParameters();
		//params.setPictureSize(3096, 4128);
		params.set("SCENE_MODE_ACTION", 1);
		//params.setRotation(90);
		params.set("orientation", "portrait");
		params.set("FOCUS_MODE_INFINITY", 1);
		mCamera.setParameters(params);

		//mCamera.setParameters(params)
        // Create our Preview view and set it as the content of our activity.
        mPreview = new CameraPreview(this, mCamera);
        FrameLayout preview = (FrameLayout) findViewById(R.id.camera_preview);
        preview.addView(mPreview);

        //mCamera.stopPreview();
     // get an image from the camera
        //mCamera.takePicture(null, mPicture, null);
        timer = new Timer("captureTimer");
        timer.scheduleAtFixedRate(task, 1000, 1000);
        /*Button captureButton = (Button) findViewById(R.id.button_capture);
        captureButton.setOnClickListener(
            new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    // get an image from the camera
//                	if (mCamera != null) {
//                		mCamera.takePicture(null, null, mPicture);
//                	}
                }
            }
        );*/
		
		
//		mImageView = (ImageView) findViewById(R.id.imageView1);
//		//dispatchTakePictureIntent();
//		//takePicture();
//		//initCamera();
//        // Create our Preview view and set it as the content of our activity.
//
//		try{
//            mPreview = new Preview(this);
//            setContentView(mPreview);
//            System.out.println("hoera");
//
//            }
//        catch(RuntimeException e){
//            System.out.println(e.getMessage());
//        }
		//initCamera();
	}

	private Camera camera = null;
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
	@Override
	protected void onDestroy() {
		   super.onDestroy();
		    if (camera!=null)
		    {
		        camera.stopPreview();
		        camera.release();
		        camera=null;
		    }
	}
	
	Camera.PictureCallback mCall = new Camera.PictureCallback()  
    {  
        @Override  
        public void onPictureTaken(byte[] data, Camera camera)  
        {          
       	 System.out.println("\n\nin picture taken");
            Toast.makeText(MainActivity.this,
                    "OnPicture taken", Toast.LENGTH_LONG).show();
           Uri uriTarget = getContentResolver().insert//(Media.EXTERNAL_CONTENT_URI, image);
           (Media.EXTERNAL_CONTENT_URI, new ContentValues());

            OutputStream imageFileOS;
            try {
                imageFileOS = getContentResolver().openOutputStream(uriTarget);
                imageFileOS.write(data);
                imageFileOS.flush();
                imageFileOS.close();

                Toast.makeText(MainActivity.this,
                        "Image saved: " + uriTarget.toString(), Toast.LENGTH_LONG).show();
            }
            catch (FileNotFoundException e) {
                e.printStackTrace();
            }catch (IOException e) {
                e.printStackTrace();
            }
            //mCamera.startPreview();

            //bmp = BitmapFactory.decodeByteArray(data, 0, data.length);  
            //iv_image.setImageBitmap(bmp);  
        }  
    };	     
	
	void initCamera() {
		
		
/*		camera = Camera.open();
		Camera.Parameters p = camera.getParameters();
		//TODO: modify parameters here... p
		camera.setParameters(p);
		camera.setDisplayOrientation(0);
        Toast.makeText(MainActivity.this,
                "opened camera", Toast.LENGTH_LONG).show();
		
		//SurfaceView view = new SurfaceView(this);
		try {
			camera.setPreviewDisplay(mPreview.getHolder());
		} catch (IOException e1) {
			e1.printStackTrace();
		}
		camera.startPreview();	//preview must be started to capture
		*/

//	     while (mPreview.mCamera == null) {
//	    	 //wait for it...
//	     }
//	     System.out.println("taking picture...");
//	     mPreview.mCamera.takePicture(null, mCall, null);

		
		//camera.takePicture(null, mCall, mCall);
		//camera.stopPreview();
		//camera.release();
	}
	
	public void takePicture() {

	//here,we are making a folder named picFolder to store pics taken by the camera using this application
	        final String dir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES) + "/picFolder/"; 
	        File newdir = new File(dir); 
	        newdir.mkdirs();
	        
		    String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date());
		    String imageFileName = "JPEG_" + timeStamp + "_";
		    File storageDir = Environment.getExternalStoragePublicDirectory(
		            Environment.DIRECTORY_PICTURES);
		    File image = null;
			try {
				image = File.createTempFile(
				    imageFileName,  /* prefix */
				    ".jpg",         /* suffix */
				    storageDir      /* directory */
				);
			} catch (IOException e1) {
				e1.printStackTrace();
			}
	        
            // here,counter will be incremented each time,and the picture taken by camera will be stored as 1.jpg,2.jpg and likewise.
            ///count++;
            //String file = dir+count+".jpg";
            //File newfile = new File(file);
            try {
                image.createNewFile();
            } catch (Exception e) {
            	System.out.println("Unable to save image: " + e.toString());
            }       

            Uri outputFileUri = Uri.fromFile(image);

            Intent cameraIntent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE); 
            cameraIntent.putExtra(MediaStore.EXTRA_OUTPUT, outputFileUri);

            startActivityForResult(cameraIntent, TAKE_PHOTO_CODE);
	}
	
	//////////////////////////

	public int intPicTaken = 0;
    public Camera.PreviewCallback prevCallBack = new Camera.PreviewCallback() {
        @Override
        public void onPreviewFrame(byte[] data, Camera camera) {
            intPicTaken++;
            try {
                if(intPicTaken == 10) {
                //doTakePicture();
                }
            } catch (Exception e) {
                System.out.println("onPreviewFrame: " + e.toString());
            }
        }
    };


    // take the picture
//    public void doTakePicture() {
//        try {
//
//            mPreview.mCamera.stopPreview();
//            mPreview.mCamera.takePicture(null, null, mPicture, mPicture);
//        } catch(Exception e){
//            System.out.println("doTakePicture: " + e.toString());
//        }
//    }

    // saving the file to gallery 
    public void saveFile(Bitmap bitmap) {
        String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date());
        File mediaStorageDir = Environment.getExternalStorageDirectory();
        if (! mediaStorageDir.exists()){
            if (! mediaStorageDir.mkdirs()){
                System.out.println("saveFile: failed to create directory");
                return;
            }
        }
        try {
            String saved = MediaStore.Images.Media.insertImage(this.getContentResolver(), bitmap, "title", "description");
            Uri sdCardUri = Uri.parse("file://" + Environment.getExternalStorageDirectory());
            sendBroadcast(new Intent(Intent.ACTION_MEDIA_MOUNTED, sdCardUri));
            System.out.println("file saved");
        } catch (Exception e) {
            System.out.println("saveFile: " + e.toString());
            e.printStackTrace();
        }
    }
    ////////////////
	
	
	

}
