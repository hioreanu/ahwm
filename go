#!/bin/sh
mv ~/.Xdefaults ~/.Xdefaults.save
out() {
	mv ~/.Xdefaults.save ~/.Xdefaults
}
trap out `seq 0 20`
xinit ./xinitrc -- -fbbpp 16 2>&1 | tee errs
