# I'm still learning Makefile
# https://www.gnu.org/software/make/manual/html_node/Quick-Reference.html

executable   = bmp
sourcesC     = bmp.c bmpLib.c
testBMPs     = $(notdir $(wildcard ./bmps/*.bmp) $(wildcard ./bmps/*.BMP))
parentDir    = $(shell pwd)

$(executable): $(sourcesC) bmpLib.h
	gcc $(sourcesC) -o $(executable) -lm -Wall -Wextra -pedantic-errors

bta: # bmps to ascii arts (files for testing)
	@$(foreach testBMP,\
		$(testBMPs),\
			./$(executable) "$(parentDir)/bmps/$(testBMP)" "$(parentDir)/ascii arts/$(subst .BMP,,$(subst .bmp,,$(subst uncompressed,ascii,$(testBMP))"));)

clean:
	@rm -rf $(executable)