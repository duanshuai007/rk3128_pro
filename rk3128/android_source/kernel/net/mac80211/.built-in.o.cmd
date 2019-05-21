cmd_net/mac80211/built-in.o :=  arm-eabi-ld -EL    -r -o net/mac80211/built-in.o net/mac80211/mac80211.o ; scripts/mod/modpost net/mac80211/built-in.o
