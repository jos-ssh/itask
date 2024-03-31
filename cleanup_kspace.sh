for file in `find inc/ kern/ lib/ -type f`; do
  sed -i 's/#ifdef CONFIG_KSPACE/#if 0 \/* CONFIG_KSPACE *\//' $file
done
