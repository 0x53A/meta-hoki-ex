# Remove the unused GNSS HAL and garden diagnostics from the vendor image.
inherit hoki-gnss-prune
PR:append:hoki = ".hokignss1"

FILESEXTRAPATHS:prepend:hoki := "${THISDIR}/files:"
SRC_URI:append:hoki = " file://hoki-location-link-libs.json"

python do_populate_sysroot:append:hoki() {
    import hashlib
    import json
    import os
    import shutil
    manifest = os.path.join(d.getVar('UNPACKDIR'), 'hoki-location-link-libs.json')
    with open(manifest) as stream:
        libraries = json.load(stream)
    output = d.getVar('SYSROOT_DESTDIR') + d.getVar('datadir') + '/hoki-location-link-libs'
    bb.utils.mkdirhier(output)
    for name, info in libraries.items():
        source = os.path.join(d.getVar('D'), info['source'])
        with open(source, 'rb') as stream:
            contents = stream.read()
        if len(contents) != info['size'] or hashlib.sha256(contents).hexdigest() != info['sha256']:
            bb.fatal('Hoki GPS vendor link input differs from validated archive: ' + name)
        shutil.copyfile(source, os.path.join(output, name))
}
