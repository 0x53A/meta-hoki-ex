# Remove the unused GNSS HAL and garden diagnostics from the vendor image.
inherit hoki-gnss-prune
PR:append:hoki = ".hokignss1"
