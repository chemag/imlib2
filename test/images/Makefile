# Makefile for generation of the test images
# Some images are not reproduced exactly so therefore they are committed to git.
# Unstable images (change when make is rerun):
# image-alpha-64-gray.png	# convert

  IMG_REF = image-ref.svg

 CONVERT        = convert
 CONVERT_BITMAP = $(CONVERT) -dither Floyd-Steinberg -colors 2

 IMCONV = imlib2_conv

 FFMPEG = ffmpeg -y -v -1

 TYPES_NOALP += .png
 TYPES_NOALP += .avif
 TYPES_NOALP += .argb
 TYPES_NOALP += .bmp
 TYPES_NOALP += .ff
 TYPES_NOALP += .heif
 TYPES_NOALP += .gif
 TYPES_NOALP += .ico
 TYPES_NOALP += .jpg
 TYPES_NOALP += .j2k -gray.j2k
 TYPES_NOALP += .jp2 -gray.jp2
 TYPES_NOALP += .jxl
#TYPES_NOALP += .lbm
 TYPES_NOALP += .ilbm
 TYPES_NOALP += .ppm -P3.ppm -P7_332.ppm
 TYPES_NOALP += .pgm -P2.pgm
 TYPES_NOALP += .pbm -P1.pbm
 TYPES_NOALP += .pam -gray.pam -mono.pam
 TYPES_NOALP += .qoi
 TYPES_NOALP += .svg
 TYPES_NOALP += .tga
 TYPES_NOALP += .tiff
 TYPES_NOALP += .webp
 TYPES_NOALP += .xbm
 TYPES_NOALP += .xpm
 TYPES_NOALP += -yuv444p.y4m -yuv422p.y4m -yuv420p.y4m
#TYPES_NOALP += .bz2
#TYPES_NOALP += .xz
#TYPES_NOALP += .gz
#TYPES_NOALP += .id3

 TYPES_ALPHA += .avif
 TYPES_ALPHA += .ff
 TYPES_ALPHA += .gif
 TYPES_ALPHA += .heif
 TYPES_ALPHA += .png -gray.png
 TYPES_ALPHA += .jp2 -gray.jp2
 TYPES_ALPHA += .j2k -gray.j2k
 TYPES_ALPHA += .pam -gray.pam -mono.pam
 TYPES_ALPHA += -P8.ppm
 TYPES_ALPHA += .svg

.PHONY: all imgs_noalp_64 imgs_alpha_64
all: imgs_noalp_64 imgs_alpha_64

 TYPES_NOALP_64 = $(TYPES_NOALP) .ff.bz2 .ff.gz .ff.xz .png.mp3
 ALL_NOALP_64  = $(addprefix image-noalp-64, $(TYPES_NOALP_64))
imgs_noalp_64:  $(ALL_NOALP_64)

 TYPES_ALPHA_64 = $(TYPES_ALPHA)
 ALL_ALPHA_64  = $(addprefix image-alpha-64, $(TYPES_ALPHA_64))

imgs_alpha_64: $(ALL_ALPHA_64)

# Not sure how to generate
 ALL_MORE += icon-*.y4m
 ALL_MORE += img-*.y4m
 ALL_MORE += icon-128*.ico


# Reference images
image-alpha-64.svg: $(IMG_REF)
	perl -p -e 's/svg width="400" height="400"/svg width="64" height="64"/' $< > $@

image-noalp-64.svg: image-alpha-64.svg
	perl -p -e 's/xrect/rect/' $< > $@

#image-noalp-64.png: image-noalp-64.svg
#	$(CONVERT) -define png:color-type=2 $< $@

%.png: %.svg
	$(IMCONV) $< $@

%.argb: %.png
	$(IMCONV) $< $@

%.avif: %.png
	avifenc -o $@ $<

%.bmp: %.png
	$(CONVERT) $< $@

%.ff: %.png
	$(IMCONV) $< $@

%.gif: %.png
	$(CONVERT) $< $@

%.heif: %.png
	heif-enc -o $@ $<

%.ico: %.png
	$(CONVERT) $< $@

%.jpg: %.png
	$(CONVERT) $< $@

%-gray.png: %.png
	$(CONVERT) -colorspace Gray $< $@

%-gray.pam: %.png
	$(CONVERT) -colorspace Gray $< $@

%-mono.pam: %-gray.pam
	pamthreshold $< > $@

%.jp2: %.png
	opj2_compress -i $< -o $@

%.j2k: %.png
	opj2_compress -i $< -o $@

%.jxl: %.png
	cjxl $< $@

%.ilbm: %.png
	$(CONVERT) $< $@

%.ppm: %.png
	$(CONVERT) $< $@

%.pgm: %.png
	$(CONVERT) $< $@

%.pbm: %.png
	$(CONVERT_BITMAP) $< $@

%.pam: %.png
	$(CONVERT) $< $@

%-P1.pbm: %.pbm
	pnmtopnm -plain <$< >$@

%-P2.pgm: %.pgm
	pnmtopnm -plain <$< >$@

%-P3.ppm: %.ppm
	pnmtopnm -plain <$< >$@

%-P8.ppm: %.png
	$(IMCONV) $< $@

%-P7_332.ppm: %.ppm
	pamtoxvmini <$< >$@

%.qoi: %.png
	$(CONVERT) $< $@

%.tga: %.png
	$(CONVERT) $< $@
#	$(CONVERT) -flip $< $@	# Bug in convert?

%.tiff: %.png
	$(CONVERT) $< $@

%.webp: %.png
	$(CONVERT) $< $@

%.xbm: %.png
	$(CONVERT_BITMAP) $< $@

%-yuv444p.y4m: %.png
	$(FFMPEG) -i $< -pix_fmt yuv444p $@
%-yuv422p.y4m: %.png
	$(FFMPEG) -i $< -pix_fmt yuv422p $@
%-yuv420p.y4m: %.png
	$(FFMPEG) -i $< -pix_fmt yuv420p $@

%.xpm: %.png
	$(CONVERT) $< $@

%.bz2: %
	bzip2 -c $< > $@
%.xz: %
	xz -c $< > $@
%.gz: %
	gzip -n -c $< > $@

%.png.mp3: %.png null.wav
	$(FFMPEG) -i null.wav -i $< -map 0 -map 1 $@

.INTERMEDIATE: null.wav
null.wav:
	sox -n -r 44100 -c 2 null.wav trim 0.0 0.0

.PHONY: clean
clean:
	rm -f $(ALL_NOALP_64) $(ALL_ALPHA_64)
	rm -f $(ALL_MORE)
	git checkout -- $(ALL_MORE)
