#!/usr/bin/make

doc: po4a

clean: po4a-clean

.PHONY: update-po po4a
update-po:
	po4a --previous --no-backups --force --no-translations po4a.conf

po4a-clean:
	po4a --previous --rm-backups --rm-translations po4a.conf

po4a:
	po4a --previous --no-backups po4a.conf
