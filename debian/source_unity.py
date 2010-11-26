import os, apport.packaging, apport.hookutils

def add_info(report, ui):
	report.add_hooks_info(ui, srcpackage='compiz')

