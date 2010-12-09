import apport.packaging

def add_info(report, ui):
	# the crash is not in the unity code so reassign
	if report.has_key("Stacktrace") and "/usr/lib/indicators" in report["Stacktrace"]:
		for words in report["Stacktrace"].split():
			if words.startswith("/usr/lib/indicators"):
				report.add_package_info(apport.packaging.get_file_package(words))
				return

	report.add_hooks_info(ui, srcpackage='compiz')

