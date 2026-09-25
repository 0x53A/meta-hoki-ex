# Hoki uses the LocationAPI helper, never the Android GNSS HAL or garden tests.
# Keep the Qualcomm engine/QMI libraries and shared HIDL interface libraries.
python hoki_prune_gnss_hal() {
    import glob
    import os
    import xml.etree.ElementTree as ET

    if d.getVar('MACHINE') != 'hoki':
        return
    dest = d.getVar('D')
    for prefix in ('', '/usr/libexec/hal-droid'):
        for partition in ('system', 'vendor'):
            root = dest + prefix + '/' + partition
            patterns = (
                'bin/hw/android.hardware.gnss@*',
                'lib/hw/android.hardware.gnss@*',
                'lib64/hw/android.hardware.gnss@*',
                'etc/init/android.hardware.gnss@*.rc',
                'bin/garden_app', 'lib/libgarden.so', 'lib64/libgarden.so',
            )
            for pattern in patterns:
                for path in glob.glob(os.path.join(root, pattern)):
                    bb.note('Removing unused Hoki GNSS HAL file: ' + path)
                    os.unlink(path)
            # Do not advertise a removed HAL or require it in a framework matrix.
            for path in glob.glob(root + '/etc/vintf/**/*.xml', recursive=True) + glob.glob(root + '/etc/manifest.xml'):
                tree = ET.parse(path)
                changed = False
                for parent in tree.iter():
                    for child in list(parent):
                        if child.tag == 'hal' and child.findtext('name') == 'android.hardware.gnss':
                            parent.remove(child)
                            changed = True
                if changed:
                    tree.write(path, encoding='utf-8', xml_declaration=True)
}
do_install[postfuncs] += " hoki_prune_gnss_hal"
