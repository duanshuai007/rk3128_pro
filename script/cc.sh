#!/bin/bash
CONFIG_H=$1
CONFILES=$2
sed -i "/define/w ${CONFILES}" ${CONFIG_H}
sed -i '/^\/\//d' ${CONFILES}
sed -i '/\\/d' ${CONFILES}
sed -i '/\//d' ${CONFILES}
sed -i 's/^#define//' ${CONFILES}
sed -i '/DEBUG/d' ${CONFILES}
totalNum=`cat ${CONFILES} | wc -l`
i=1
while((${i} <= ${totalNum}))
do
    size=`cat ${CONFILES} | sed -n "${i}p" | wc -w`
    if [ ${size} -lt 2 ];
    then
        sed -i "${i}d" ${CONFILES}
        totalNum=`expr ${totalNum} - 1`
    else
        i=`expr ${i} + 1`
    fi
done
sed -i 's/^[ \t]*//g' ${CONFILES}
sed -i 's/ \{1,80\}/=/g' ${CONFILES}
