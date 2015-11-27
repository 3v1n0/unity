import apport.packaging
from apport.hookutils import *

def add_info(report, ui):

    # for install from the ppa
    if not apport.packaging.is_distro_package(report['Package'].split()[0]):
        report['CrashDB'] = 'unity'
        try:
            version = packaging.get_version('unity')
        except ValueError:
            version = 'N/A'
        if version is None:
            version = 'N/A'
        report['Tags'] += " rc-%s" % version
    
    # the crash is not in the unity code so reassign
    if "Stacktrace" in report and "/usr/lib/indicators" in report["Stacktrace"]:
        for words in report["Stacktrace"].split():
            if words.startswith("/usr/lib/indicators"):
                report.add_package_info(apport.packaging.get_file_package(words))
                return

    # Include the compiz details
    report.add_hooks_info(ui, srcpackage='compiz')
    # the upstart logs
    attach_upstart_logs(report, 'unity-services')
    attach_upstart_logs(report ,'libunity-core-6.0-9')
    # some gsettings configs
    attach_gsettings_schema(report, 'com.canonical.Unity')
    attach_gsettings_schema(report, 'com.ubuntu.user-interface')
    attach_gsettings_schema(report, 'org.gnome.desktop.interface')
    attach_gsettings_schema(report, 'org.gnome.desktop.lockdown')
