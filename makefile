all: 
	gcc -Wall talk.c `pkg-config fuse3 --cflags --libs` -o talk
run:
	mkdir TalkRoom
	./talk TalkRoom

clean:
	umount TalkRoom
	rm talk
	rm -rf TalkRoom
test:
	mkdir -p TalkRoom/Alice TalkRoom/Bob TalkRoom/Cleo TalkRoom/Alice/Friends TalkRoom/Bob/Friends TalkRoom/Bob/Classmates TalkRoom/Cleo/Classmates
	touch TalkRoom/Alice/Friends/Bob TalkRoom/Bob/Friends/Alice TalkRoom/Alice/Cleo TalkRoom/Bob/Classmates/Cleo TalkRoom/Cleo/Classmates/Bob TalkRoom/Cleo/Alice