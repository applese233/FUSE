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
	mkdir TalkRoom/Alice TalkRoom/Bob TalkRoom/Cleo TalkRoom/Alice/Friends TalkRoom/Bob/Sons TalkRoom/Bob/Baby
	touch TalkRoom/Alice/Friends/Bob TalkRoom/Bob/Sons/Alice TalkRoom/Alice/Cleo TalkRoom/Bob/Baby/Cleo TalkRoom/Cleo/Bob TalkRoom/Cleo/Alice