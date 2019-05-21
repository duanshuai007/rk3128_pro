cmd_pie/pie_stage2.o := arm-eabi-ld -EL   --no-undefined -X  -r --defsym=_GLOBAL_OFFSET_TABLE_=_GLOBAL_OFFSET_TABLE_ pie/pie_stage1.o pie/libpie_stage2.o -o pie/pie_stage2.o 
