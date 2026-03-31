build:
	@gcc main.c -o ascii_player -lavformat -lavutil -lavcodec -lswscale
	@ffmpeg -y -f lavfi -i testsrc=duration=10:size=1280x720:rate=30 test.mp4
	@clear
clean:
	@rm ascii_player test.mp4
	@clear