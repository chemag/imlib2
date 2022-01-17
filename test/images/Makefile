# Makefile for generation of the test images
# Some images are not reproduced exactly so therefore they are committed to git.

 TYPES += argb
 TYPES += bmp
 TYPES += ff
 TYPES += gif
 TYPES += ico
 TYPES += jpg
#TYPES += lbm
 TYPES += ilbm
#TYPES += png
 TYPES += ppm
 TYPES += pgm
 TYPES += pbm
 TYPES += tga
 TYPES += tiff
 TYPES += webp
 TYPES += xbm
 TYPES += xpm
#TYPES += bz2
#TYPES += xz
#TYPES += gz
#TYPES += id3

.PHONY: all imgs_64
all: imgs_64

 TYPES_64 = $(filter-out png, $(TYPES)) ff.bz2 ff.gz ff.xz # png.mp3
 ALL_64  = $(addprefix icon-64., $(TYPES_64))
$(warning imgs_64=$(ALL_64))
imgs_64:  $(ALL_64)

%.argb: %.png
	imlib2_conv $< $@

%.bmp: %.png
	convert $< $@

%.ff: %.png
	imlib2_conv $< $@

%.gif: %.png
	convert $< $@

%.ico: %.png
	convert $< $@

%.jpg: %.png
	convert $< $@

%.ilbm: %.png
	convert $< $@

%.ppm: %.png
	convert $< $@

%.pgm: %.png
	convert $< $@

%.pbm: %.png
	convert -dither Floyd-Steinberg $< $@

%.tga: %.png
#	convert $< $@
	convert -flip $< $@	# Bug in convert?

%.tiff: %.png
	convert $< $@

%.webp: %.png
	convert $< $@

%.xbm: %.png
	convert -dither Floyd-Steinberg $< $@

%.xpm: %.png
	convert $< $@

%.bz2: %
	bzip2 -c $< > $@
%.xz: %
	xz -c $< > $@
%.gz: %
	gzip -c $< > $@

.PHONY: clean
clean:
	rm -f $(ALL_64)
