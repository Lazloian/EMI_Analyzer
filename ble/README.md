## External Antenna

Amazon link: https://www.amazon.com/gp/product/B08LVH5BCP/ref=ppx_yo_dt_b_asin_title_o00_s00?ie=UTF8&psc=1

It could be other brands as long as the chipset is RTL8761B

External antenna driver link: https://www.dropbox.com/s/ccikksvadzunzi7/20201202_mpow_BH456A_driver%2Bfor%2BLinux.7z

Backup link: https://purdue0-my.sharepoint.com/:u:/g/personal/tbureete_purdue_edu/EegK6irMO7FBuqLpV9eoppYBLGSkI21x6A2ySZxq1EHgMw?e=feRgzX

Move driver files as below

<code>
  rtkbt-firmware/lib/firmware/rtlbt/rtl8761b_fw /lib/firmware/rtl_bt/rtl8761b_fw.bin
  
  rtkbt-firmware/lib/firmware/rtlbt/rtl8761b_config /lib/firmware/rtl_bt/rtl8761b_config.bin
</code>

Disable the default antenna by adding <code>dtoverlay=disable-bt</code> into <code>/boot/config.txt</code>
