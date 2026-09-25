# Remove the stock GNSS launcher from the patched Android runtime.
inherit hoki-gnss-prune
PR:append:hoki = ".hokignss1"
