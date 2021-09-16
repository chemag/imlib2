#!/bin/sh

#GLO="git log --format=-%x20%s"
#GLO="git shortlog -w76,4,6"
GLO="git shortlog"

VT=`git tag --sort=-version:refname -l`
VH=""
if [ "$1" != "" ]; then VH="HEAD"; fi
V2=""
for V1 in $1 $VT
do
    if [ "$V2" != "" ]; then
        if [ "$VH" = "HEAD" ]; then
            VD="$VH"
        else
            VD="$V2"
            VH="$V2"
        fi
        D=$(git show -s --format=%cs $VD^{commit})
        printf "%s - %s\n-------------------\n" "$V2" "$D"
        $GLO $V1..$VH
        VH=""
    fi
    V2="$V1"
done

D=$(git show -s --format=%cs $VD^{commit})
printf "\n%s - %s (from dawn of time)\n---------------------------------------\n" "$V2" "$D"
$GLO $V2
