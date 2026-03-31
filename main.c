#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <libavutil/log.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

int main(int argc, char* argv[]){
    av_log_set_level(AV_LOG_ERROR);
    if (argc < 2) { //check parameter correction
        printf("Usage: %s <video_path>\n", argv[0]);
        return -1;
    }

    AVFormatContext* format_ctx = avformat_alloc_context(); //memory allocation

    if(avformat_open_input(&format_ctx, argv[1], NULL, NULL) < 0){
        printf("Error : Can't open video file %s!\n", argv[1]);
        return -1;
    }

    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        printf("Error: Can't find stream information!\n");
        avformat_close_input(&format_ctx);
        return -1;
    }

    printf("Duration : %ld s\n", (format_ctx->duration) / AV_TIME_BASE); //AV_TIME_BASE = 1,000,000

    int video_stream_index = -1;
    AVCodecParameters *codec_par = NULL;
    AVCodecContext *codec_ctx = NULL;
    const AVCodec *codec = NULL;

    for(int i = 0; i < format_ctx->nb_streams; i++){ // traverse stream
        if(format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){ // find video stream
            video_stream_index = i;
            codec_par = format_ctx->streams[i]->codecpar; // get codec parameters
            break;
        }
    }

    if (video_stream_index == -1) {
        printf("Error: Can't find video stream！\n");
        return -1;
    }

    codec = avcodec_find_decoder(codec_par->codec_id);
    if(!codec){
        printf("Error : Can't find codec!\n");
        return -1;
    }

    codec_ctx = avcodec_alloc_context3(codec); //allocate codec work environment
    if(avcodec_parameters_to_context(codec_ctx, codec_par) < 0){ //copy parameters to work environment
        printf("Error : Can't copy codec parameters!\n");
        return -1;
    }

    if (avcodec_open2(codec_ctx, codec, NULL) < 0){ //open codec
        printf("Error: Can't open codec！\n");
        return -1;
    }
    printf("Resolution : %d x %d\n", codec_ctx->width, codec_ctx->height);

    AVRational frame_rate = format_ctx->streams[video_stream_index]->avg_frame_rate;
    double fps = (double)frame_rate.num / frame_rate.den;
    printf("FPS : %.2f\n", fps);
    int frame_delay_us = (int)(1000000.0 / fps);

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    if (!packet || !frame) {
        printf("Error: Can't allocate packet or frame！\n");
        return -1;
    }

    double font_aspect_ratio = 2.1; 
    double original_width = codec_ctx->width;
    double original_height = codec_ctx->height;
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width = w.ws_col;
    int term_height = w.ws_row - 1;
    double effective_video_ratio = (original_width / original_height) * font_aspect_ratio;
    double terminal_ratio = (double)term_width / term_height;
    int target_width, target_height;
    if (effective_video_ratio > terminal_ratio) {
        target_width = term_width;
        target_height = (int)(target_width / effective_video_ratio);
    } else {
        target_height = term_height;
        target_width = (int)(target_height * effective_video_ratio);
    }

    if (target_width <= 0) target_width = 1;
    if (target_height <= 0) target_height = 1;
    
    struct SwsContext *sws_ctx = sws_getContext(
        codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt, 
        target_width, target_height, AV_PIX_FMT_GRAY8,           
        SWS_BILINEAR,                                           
        NULL, NULL, NULL                                         
    );

    if (!sws_ctx) { 
        printf("Error: Can't initialize libswscale！\n");
        return -1;
    }

    AVFrame* frame_resized = av_frame_alloc();
    if(!frame_resized){
        printf("Error : Can't allocate frame!\n");
        return -1;
    }
    frame_resized->format = AV_PIX_FMT_GRAY8;
    frame_resized->width = target_width;
    frame_resized->height = target_height;

    int ret = av_image_alloc(
    frame_resized->data,      
    frame_resized->linesize,  
    target_width,            
    target_height,            
    AV_PIX_FMT_GRAY8,         
    32                        
    );

    if (ret < 0) {
        printf("Error: Can't allocate resized memory！\n");
        return -1;
    }

    while(av_read_frame(format_ctx, packet) >= 0){
        if(packet->stream_index == video_stream_index){
            int response = avcodec_send_packet(codec_ctx, packet);
            if(response < 0){
                printf("Error : Can't send packet to codec!\n");
                break;
            }
            while(response >= 0){
                response = avcodec_receive_frame(codec_ctx, frame);
                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF){
                    break;
                } 
                else if (response < 0){
                    printf("Error: Can't receive from codec！\n");
                    break;
                }
                sws_scale(
                    sws_ctx, 
                    (uint8_t const * const *)frame->data,
                    frame->linesize, 
                    0,
                    codec_ctx->height,
                    frame_resized->data, 
                    frame_resized->linesize
                );

                const char *ascii_chars = " .:-=+*#%@";
                printf("\033[H");
                for (int y = 0; y < target_height; y++) {
                    for (int x = 0; x < target_width; x++) {
                        int pixel_index = y * frame_resized->linesize[0] + x;
                        uint8_t pixel_value = frame_resized->data[0][pixel_index];
                        int char_index = pixel_value * 9 / 255;
                        putchar(ascii_chars[char_index]);
                    }
                    putchar('\n');
                }
                usleep(frame_delay_us);
            }
        }
        av_packet_unref(packet);
    }

    av_freep(&frame_resized->data[0]); 
    av_frame_free(&frame_resized);     
    sws_freeContext(sws_ctx);
    av_frame_free(&frame);
    av_packet_free(&packet); 
    avformat_close_input(&format_ctx);
    return 0;
}