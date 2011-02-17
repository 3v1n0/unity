import apport.packaging

def add_info(report, ui):
    
    # the crash is not in the unity code so reassign
    if report.has_key("Stacktrace") and "/usr/lib/indicators" in report["Stacktrace"]:
        for words in report["Stacktrace"].split():
            if words.startswith("/usr/lib/indicators"):
                report.add_package_info(apport.packaging.get_file_package(words))
                return

    # only reports all compiz infos if a graphical bug
    compiz_bug = False
    if ui and report['SourcePackage'] == "unity":
        if ui.yesno("Thanks for reporting this bug on unity.  Is the issue you are reporting graphical (will report more information about your graphic configuration)?"):
            compiz_bug = True
    if compiz_bug:
        report.add_hooks_info(ui, srcpackage='compiz')
    else:
        # still send some info like the plugins activated
        # Plugins
        report['CompizPlugins'] = command_output(['gconftool-2',
            '--get', '/apps/compiz-1/general/screen0/options/active_plugins'])

        # User configuration
        report['GconfCompiz'] = command_output(['gconftool-2', '-R', '/apps/compiz-1'])
