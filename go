#!/bin/sh
mv ~/.Xdefaults ~/.Xdefaults.save
out() {
	mv ~/.Xdefaults.save ~/.Xdefaults
}
trap out `seq 0 20`
rm -f core
xinit ./xinitrc -- :1 -fbbpp 24 2>&1 | tee errs
