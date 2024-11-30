#include <jni.h>
#include <opencv2/opencv.hpp>
#include <android/bitmap.h>
#include <vector>
#include <android/log.h>

using namespace std;

void bitmapToMat(JNIEnv * env, jobject bitmap, cv::Mat &dst, jboolean
needUnPremultiplyAlpha){
    AndroidBitmapInfo  info;
    void*       pixels = 0;

    try {
        CV_Assert( AndroidBitmap_getInfo(env, bitmap, &info) >= 0 );
        CV_Assert( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                   info.format == ANDROID_BITMAP_FORMAT_RGB_565 );
        CV_Assert( AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0 );
        CV_Assert( pixels );
        dst.create(info.height, info.width, CV_8UC4);
        if( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 )
        {
            cv::Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if(needUnPremultiplyAlpha) cvtColor(tmp, dst, cv::COLOR_mRGBA2RGBA);
            else tmp.copyTo(dst);
        } else {
            // info.format == ANDROID_BITMAP_FORMAT_RGB_565
            cv::Mat tmp(info.height, info.width, CV_8UC2, pixels);
            cvtColor(tmp, dst, cv::COLOR_BGR5652RGBA);
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch(const cv::Exception& e) {
        AndroidBitmap_unlockPixels(env, bitmap);
        //jclass je = env->FindClass("org/opencv/core/CvException");
        jclass je = env->FindClass("java/lang/Exception");
        //if(!je) je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nBitmapToMat}");
        return;
    }
}


void matToBitmap(JNIEnv * env, cv::Mat src, jobject bitmap, jboolean needPremultiplyAlpha) {
    AndroidBitmapInfo  info;
    void*              pixels = 0;
    try {
        CV_Assert( AndroidBitmap_getInfo(env, bitmap, &info) >= 0 );
        CV_Assert( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 ||
                   info.format == ANDROID_BITMAP_FORMAT_RGB_565 );
        CV_Assert( src.dims == 2 && info.height == (uint32_t)src.rows && info.width ==
                                                                         (uint32_t)src.cols );
        CV_Assert( src.type() == CV_8UC1 || src.type() == CV_8UC3 || src.type() == CV_8UC4 );
        CV_Assert( AndroidBitmap_lockPixels(env, bitmap, &pixels) >= 0 );
        CV_Assert( pixels );
        if( info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 )
        {
            cv::Mat tmp(info.height, info.width, CV_8UC4, pixels);
            if(src.type() == CV_8UC1)
            {
                cvtColor(src, tmp, cv::COLOR_GRAY2RGBA);
            } else if(src.type() == CV_8UC3){
                cvtColor(src, tmp, cv::COLOR_RGB2RGBA);
            } else if(src.type() == CV_8UC4){
                if(needPremultiplyAlpha) cvtColor(src, tmp, cv::COLOR_RGBA2mRGBA);
                else src.copyTo(tmp);
            }
        } else {
            // info.format == ANDROID_BITMAP_FORMAT_RGB_565
            cv::Mat tmp(info.height, info.width, CV_8UC2, pixels);
            if(src.type() == CV_8UC1)
            {
                cvtColor(src, tmp, cv::COLOR_GRAY2BGR565);
            } else if(src.type() == CV_8UC3){
                cvtColor(src, tmp, cv::COLOR_RGB2BGR565);
            } else if(src.type() == CV_8UC4){
                cvtColor(src, tmp, cv::COLOR_RGBA2BGR565);
            }
        }
        AndroidBitmap_unlockPixels(env, bitmap);
        return;
    } catch(const cv::Exception& e) {
        AndroidBitmap_unlockPixels(env, bitmap);
        //jclass je = env->FindClass("org/opencv/core/CvException");
        jclass je = env->FindClass("java/lang/Exception");
        //if(!je) je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        return;
    } catch (...) {
        AndroidBitmap_unlockPixels(env, bitmap);
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown exception in JNI code {nMatToBitmap}");
        return;
    }
}


extern "C" JNIEXPORT void JNICALL
Java_vision_applicacionnativa_MainActivity_filtroSketch(JNIEnv* env, jobject /*this*/, jobject bitmapIn, jobject bitmapOut){

    cv::Mat frame;
    cv::Mat areaEfecto, imgGray, inverted, blurred, inverted_blur, filtroSketch, bordes;

    // Convertir el bitmap de entrada a Mat
    bitmapToMat(env, bitmapIn, frame, false);


    // Reducir el ruido con filtro de la mediana
    cv::medianBlur(frame, frame, 3);

    // Definimo el ROI en el centro de la imagen, esto nos sirve para aplicar el filtro Sketch en un área específica
    int areaEfecto_width = 150;
    int areaEfecto_height = 150;
    int x_center = (frame.cols - areaEfecto_width) / 2;
    int y_center = (frame.rows - areaEfecto_height) / 2;
    cv::Rect roi(x_center, y_center, areaEfecto_width, areaEfecto_height);

    //Con el ROI que se creo extraemos el área en la cuál se aplicará el filtro Sketch
    areaEfecto = frame(roi);

    //****************** Desde este punto manejamos un área específica de la imagen para el efecto Sketch ******************
    // Conversión a escala de grises
    cv::cvtColor(areaEfecto, imgGray, cv::COLOR_BGR2GRAY);

    // Aplicamos la técnica Unsharp Masking
    cv::Mat gaussianUnsharp;
    cv::GaussianBlur(imgGray, gaussianUnsharp, cv::Size(5, 5), 1.5);

    cv::Mat unsharp_image;
    cv::addWeighted(imgGray, 1.8, gaussianUnsharp, -0.8, 0, unsharp_image);

    imgGray = unsharp_image;

    //Mejoramos los problemas problemas de iluminación con CLAHE
    cv::Ptr<cv::CLAHE> clahe = createCLAHE(2, cv::Size(3,3));
    clahe->apply(imgGray, imgGray);

    //Aplicamos operaciones morfológicas para eliminar el ruido, suavizar los bordes
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(imgGray, imgGray, cv::MORPH_OPEN, kernel);

    // Invertimos la imagen en escala de grises
    cv::bitwise_not(imgGray, inverted);

    // Difuminamos la imagen invertida para suavizar los bordes
    cv::GaussianBlur(inverted, blurred, cv::Size(21, 21), 0);

    // Invertimos la imagen difuminada
    cv::bitwise_not(blurred, inverted_blur);

    // Realizamos una división para crear el efecto de bosquejo
    cv::divide(imgGray, inverted_blur, filtroSketch, 256.0);

    // Suavizamos el bosquejo
    cv::GaussianBlur(filtroSketch, filtroSketch, cv::Size(5, 5), 0);

    // Aplicamos la detección de bordes Canny
    cv::Canny(imgGray, bordes, 100,  100*1.3, 3);

    // Convertimos la imagen de bordes a 4 canales
    cv::Mat bordes_colores;
    cv::cvtColor(bordes, bordes_colores, cv::COLOR_GRAY2BGRA);

    // Redimensionamos ambas imágenes para que tengan el mismo tamaño
    cv::resize(filtroSketch, filtroSketch, bordes_colores.size());

    // Verificamos que ambas imágenes tengan el mismo número de canales
    if (filtroSketch.channels() == 1) {
        cv::cvtColor(filtroSketch, filtroSketch, cv::COLOR_GRAY2BGRA);
    }

    // Combinamos el efecto de bosquejo con la detección de bordes
    cv::Mat imgCombined = filtroSketch + bordes_colores;

    //Sobrescribimos el ROI en el frame original con el resultado combinado
    imgCombined.copyTo(frame(roi));

    // Convertimos la imagen resultante a bitmap
    matToBitmap(env, frame, bitmapOut, false);
}

////cv::morphologyEx(imgGray, imgGray, cv::MORPH_OPEN, kernel);  esto aplicamos en la linea 146