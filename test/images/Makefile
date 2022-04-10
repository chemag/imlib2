# Makefile for generation of the test images
# Some images are not reproduced exactly so therefore they are committed to git.

 TYPES += argb
 TYPES += bmp
 TYPES += ff
 TYPES += heif
 TYPES += gif
 TYPES += ico
 TYPES += jpg
 TYPES += j2k
 TYPES += jp2
 TYPES += jxl
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

.PHONY: all imgs_64 imgs_misc
all: imgs_64 imgs_misc

 TYPES_64 = $(filter-out png, $(TYPES)) ff.bz2 ff.gz ff.xz # png.mp3
 ALL_64  = $(addprefix icon-64., $(TYPES_64))
 ALL_64 += icon-64-P1.pbm icon-64-P2.pgm icon-64-P3.ppm icon-64-P7_332.ppm
$(warning imgs_64=$(ALL_64))
imgs_64:  $(ALL_64)

 ALL_MISC  = icon-64-gray.jp2 icon-64-gray.j2k
 ALL_MISC += xeyes.jp2 xeyes.j2k
 ALL_MISC += xeyes-gray.png
 ALL_MISC += xeyes-gray.jp2 xeyes-gray.j2k
imgs_misc: $(ALL_MISC)

%.argb: %.png
	imlib2_conv $< $@

%.bmp: %.png
	convert $< $@

%.ff: %.png
	imlib2_conv $< $@

%.gif: %.png
	convert $< $@

%.heif: %.png
	heif-enc -o $@ $<

%.ico: %.png
	convert $< $@

%.jpg: %.png
	convert $< $@

%-gray.png: %.png
	convert -colorspace Gray $< $@

%.jp2: %.png
	opj2_compress -i $< -o $@

%.j2k: %.png
	opj2_compress -i $< -o $@

%.jxl: %.png
	cjxl $< $@

%.ilbm: %.png
	convert $< $@

%.ppm: %.png
	convert $< $@

%.pgm: %.png
	convert $< $@

%.pbm: %.png
	convert -dither Floyd-Steinberg $< $@

%-P1.pbm: %.pbm
	pnmtopnm -plain <$< >$@

%-P2.pgm: %.pgm
	pnmtopnm -plain <$< >$@

%-P3.ppm: %.ppm
	pnmtopnm -plain <$< >$@

%-P7_332.ppm: %.ppm
	pamtoxvmini <$< >$@

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
	gzip -n -c $< > $@

.PHONY: clean
clean:
	rm -f $(ALL_64) $(ALL_MISC)
