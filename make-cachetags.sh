#!/bin/sh
set -euo pipefail
set -x

mkco() {
	mkdir -p "$1"
	echo 'U2lnbmF0dXJlOiA4YTQ3N2Y1OTdkMjhkMTcyNzg5ZjA2ODg2ODA2YmM1NQojIFRoaXMgZmlsZSBpcyBhIGNhY2hlIGRpcmVjdG9yeSB0YWcgY3JlYXRlZCBieSBmb250Y29uZmlnLgojIEZvciBpbmZvcm1hdGlvbiBhYm91dCBjYWNoZSBkaXJlY3RvcnkgdGFncywgc2VlOgojICAgICAgIGh0dHA6Ly93d3cuYnJ5bm9zYXVydXMuY29tL2NhY2hlZGlyLwo=' | base64 -d > "$1/CACHEDIR.TAG"
}

if [ "$(id -u)" != "0" ]; then
	mkco "$HOME/.cache"
	exit 0
fi

mkco /var/cache
mkco /mnt/tmpdrv
