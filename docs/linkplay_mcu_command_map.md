# Linkplay MCU Command Map

Bu belge STM32DSP1466 firmware'inde kullanilan Linkplay/MCU komutlarini,
AX31 reverse engineering notlarini ve A98 MCU instruction PDF'inden cikan ek
komutlari tek yerde takip etmek icin hazirlandi.

Kaynak ayrimi:

- `Firmware`: Su an `src/main.cpp` icinde aktif kullanilan davranis.
- `AX31 RE`: `AX31UARTAPI.xlsx` icinde Up2Stream/A31 uzerinden yakalanan davranis.
- `A98 PDF`: `Instructiondocument.pdf` icinde anlatilan daha modern protokol
  davranisi. A31 icin dogrudan dogru kabul edilmez; anlam ve sira referansi
  olarak kullanilir.

Not: Bizim mevcut programdaki Linkplay UART ayari dogru kabul edilir. PDF'teki
genel UART ayarlari bu projedeki eski modulu gecersiz kilmaz.

## Firmware'de Su An Kullandigimiz Komutlar

| Yon | Komut | Firmware davranisi | Not |
| --- | --- | --- | --- |
| MCU -> LP | `MCU+VOL+%03u` | Mevcut volume degerini LP'ye bildirir. | `AXX+VOL+GET` cevabi olarak ve hazirlikta kullaniliyor. |
| LP -> MCU | `AXX+VOL+GET` | `MCU+VOL+%03u` ile cevaplanir. | A98 PDF ile uyumlu. |
| LP -> MCU | `AXX+VOL+nnn` | STM32 volume'u uygular ve EEPROM'a kaydeder. | `0..100` araligina sinirlanir. |
| MCU -> LP | `MCU+PLM-xxx` | Mevcut source/mode bilgisini LP'ye bildirir. | Firmware `-` formunu kullaniyor; standby/boot sirasinda EEPROM'da son source BT/XLR/Optical olsa bile `000` raporlanir. Network source icinde son servis mode'u biliniyorsa `031/010/001` gibi geri raporlanabilir. |
| LP -> MCU | `AXX+PLM+GET` | Deneysel olarak `MCU+PLM+xxx` ile cevaplanir. | Spotify standby wake dongusunde `-` formu yerine `+` formu deneniyor. Boot `MCU+VER...&MCU+PLM-xxx` ayni kalir. |
| LP -> MCU | `AXX+PLM+nnn` | Gelen LP mode'una gore source secilir. | `031 Spotify`, `001 AirPlay`, `010 Radio` Network source icindeki servis/mod ipuclaridir; DSP source yine Network kalir. `040 Line In`, `041 Bluetooth`, `043 Optical`, `048 XLR`, `000 Network`. |
| MCU -> LP | `MCU+VER+0035&MCU+PLM-xxx` | MCU versiyonunu ve mode'u birlikte bildirir. | `AXX+MCU+VER` ve `AXX+MCU+RDY` sonrasinda kullaniliyor; hemen ardindan `MCU+CAP+PLM00008214&` proaktif gonderilir. |
| LP -> MCU | `AXX+MCU+VER` | Versiyon/mode cevabi gonderilir. | A98 PDF'te normal boot sorgularindan biri. |
| LP -> MCU | `AXX+MCU+RDY` | Ilk hazir olusta versiyon, CAP, volume ve EQ bilgileri raporlanir. | Linkplay hazir sinyali; metadata hazir olayi `AXX+MEA+RDY` tarafidir. |
| MCU -> LP | `MCU+RTC+GET` | LP/network time ister. | `AXX+GET+RTC` cevabi olarak kullaniliyor; LP once `AXX+RTC+NIL`, sonra NTP geldiginde `AXX+SET+RTC...` donebilir. |
| MCU -> LP | `MCU+MEA+GET` | Metadata istenir. | Sonrasinda `MCU+PINFGET` zincirlenebilir. |
| LP -> MCU | `AXX+MEA+DAT{...}&` | Hex metadata cozulur, ekrana yazilir. | Title/Artist/Album/Vendor. |
| LP -> MCU | `AXX+MEA+RDY` | Media/player info hazir sayilir, `MCU+PINFGET` tetiklenir. | Spotify standby wake denemelerinde tekrar `MCU+MEA+GET` basmak dongu yaratabildi. |
| MCU -> LP | `MCU+PINFGET` | Player/session bilgisi istenir. | `AXX+PLY+INF{...}&` beklenir. |
| LP -> MCU | `AXX+PLY+INF{...}&` | Player info, volume, mute, sureler ve metadata islenir. | Spotify gibi kaynaklarda onemli. |
| LP -> MCU | `AXX+PLY+INF{..."loop":"0/1/3/4"...}` | Playback loop/shuffle durumu. | Firmware `0=repeat all/album`, `1=repeat one`, `3=shuffle/random` ikonlarini gosterir; `4=normal/clear` ve bilinmeyen `2` icin ikon basmaz. Mute ikonu varsa loop ikonundan once gelir. |
| MCU -> LP | `MCU+PLY+PUS` | Play/pause toggle veya standby oncesi pause. | Network/Linkplay transport icin kullaniliyor. Standby'ye girerken sadece son bilinen state `play` ise gonderilir; `pause/stop` iken gondermek Spotify'i yeniden play'e cekebilir. |
| MCU -> LP | `MCU+PLY+NXT` | Sonraki parca. | IR/terminal next icin kullaniliyor; standby ekranda da current source Network ise gonderilebilir. |
| MCU -> LP | `MCU+PLY+PRV` | Onceki parca. | IR/terminal previous icin kullaniliyor; standby ekranda da current source Network ise gonderilebilir. |
| LP -> MCU | `AXX+PLY+001` | Transport play event'i. | Network source standby wake icin ana tetiklerden biri. |
| LP -> MCU | `AXX+PLY+000` | Pause kabul edilir. | |
| LP -> MCU | `AXX+SPY+001` | Spotify session aktif bilgisi. | Cihaz seciminde de gelebilir; auto-wake tetiklemez. |
| LP -> MCU | `AXX+PLM+031` | Spotify mode bildirimi. | Kaynak olarak Network'e gecirir ve metadata ister; Spotify ikon/baslik karari `MEA+DAT` vendor veya `PLY+INF` mode bilgisinden sonra verilir. Spotify standby wake dongusunde `PLM+GET` cevabi icin `MCU+PLM+031` deneniyor. |
| LP -> MCU | `AXX+PLY+INF{..."status":"play"...}` | Gercek play state. | Standby'dan auto-wake icin ana tetik. |
| MCU -> LP | `MCU+MUT+001` / `MCU+MUT+000` | LP mute durumunu bildirir. | Firmware mute toggle'da gonderiyor. |
| LP -> MCU | `AXX+MUT+001` / `AXX+MUT+000` | STM32 mute uygular. | |
| LP -> MCU | `AXX+WWW+001` / `AXX+WWW+000` | WiFi hazir/deger durumunu takip eder. | Firmware `linkplayWifiReady` set ediyor. |
| LP -> MCU | `AXX+SET+RTCYYYYMMDDHHMMSS&` | STM32 saatini gelen UTC RTC string'i ile senkronlar. | PDF'te RTC/app zaman akisi aciklanmis. |
| LP -> MCU | `AXX+NXT+ALS<n>&` | Daylight Saving / local summer-time offset bildirimi olabilir. | `ALS-1` 1 saat DST farki, `ALS-2` 2 saat fark gibi yorumlandi. RTC set akisi sonrasi gelebiliyor; firmware su an saat uzerine ayrica uygulamiyor. |
| LP -> MCU | `AXX+BOT+DON` / `AXX+BOT+UP0` | LP boot bildirimi. | Firmware bir kez `MCU+BOT+DON` ile cevap verir. |
| LP -> MCU | `AXX+PAS+EQGet&` | Bass/treble EQ degeri raporlanir. | |
| LP -> MCU | `AXX+PAS+EQSet:bass:n&` | Bass gain'e cevrilir ve DSP'ye yazilir. | |
| LP -> MCU | `AXX+PAS+EQSet:treble:n&` | Treble gain'e cevrilir ve DSP'ye yazilir. | |
| MCU -> LP | `MCU+PAS+EQSet:bass:nn&MCU+PAS+EQSet:treble:nn&` | EQ durumunu LP/app tarafina raporlar. | AX31 V3'te cevap vermedigi not edilmis, ama firmware kullaniyor. |

## AX31 RE Dokumaninda Kanitli Komutlar

| Yon | Komut | Yakalanan cevap/davranis | Durum |
| --- | --- | --- | --- |
| MCU -> LP | `MCU+BOT+DON` | `AXX+BOT+DON`, sonra `AXX+BOT+UP0`. | Boot handshake icin kanitli. Firmware `AXX+BOT+DON/UP0` sonrasi bir kez cevapliyor. |
| MCU -> LP | `MCU+POW+SAV` | `AXX+POW+001`, ardindan `AXX+RA0+000`. | A31'de power-save/on-off davranisi gibi. Tam power-off olmayabilir. |
| MCU -> LP | `MCU+WWW+GET` | `AXX+WWW+001`. | WiFi durum sorgusu. |
| MCU -> LP | `MCU+GET+UID` | `AXX+UID+...&`. | Boot sekansinda kullanilmis. |
| MCU -> LP | `MCU+DEV+GET` | `AXX+DEV+INF...&`. | Kisa device info. |
| MCU -> LP | `MCU+INF+GET` | `AXX+INF+INF{...}&`. | Detayli device info. |
| MCU -> LP | `MCU+PLP+GET` | `AXX+PLP+004`. | Play loop/sira durumu. |
| MCU -> LP | `MCU+PLP+000` | Boot sekansinda gonderilmis. | Anlami play loop/mode raporu olabilir. |
| MCU -> LP | `MCU+EQ+GTYP` | Boot sekansinda gonderilmis. | App/EQ tipi icin olabilir. |
| MCU -> LP | `MCU+VOL+030` | Volume set eder. | Set komutu. |
| MCU -> LP | `MCU+VOL+GET` | `AXX+VOL+value`. | Module volume sorgusu. |
| MCU -> LP | `MCU+VOL-100` | `AXX+VOL+100`. | MCU'nun kendi volume degerini module bildirmesi olarak not edilmis. |
| MCU -> LP | `MCU+PLM+000` | `AXX+PLM+000`, Network. | `+` formu module source switch icin. |
| MCU -> LP | `MCU+PLM+005` | `AXX+PLM+040`, Line In. | AX31 mode mapping; Aux-In icin. |
| MCU -> LP | `MCU+PLM+006` | `AXX+PLM+041`, Bluetooth. | AX31 mode mapping. |
| MCU -> LP | `MCU+PLM+008` | `AXX+PLM+043`, Optical. | AX31 mode mapping. |
| MCU -> LP | `MCU+PLM+010` | `AXX+PLM+045`, Coaxial. | Bizde aktif kaynak degil. |
| MCU -> LP | `MCU+PLM+012` | `AXX+PLM+047`, Line In 2. | Bizde aktif kaynak degil. |
| MCU -> LP | `MCU+PLM+013` | `AXX+PLM+048`, XLR. | CAP+PLM ile XLR acildiktan sonra bizim XLR source icin dogru mapping. |
| MCU -> LP | `MCU+USB+GET` | `AXX+USB+000` / `001`. | USB device sorgusu. `001` USB bellek takili/algilandi, `000` USB bellek yok/cikarildi olarak tak-cikar testinde goruldu. Bu event source degistirme degil; aktif playback source icin `PLY+INF mode` esas alinmali. |
| LP -> MCU | `AXX+PLY+INF{..."mode":"11"...}` | USB flash playback. | Uygun formatli USB bellekten calarken goruldu. Fiziksel DSP yolu Linkplay olarak kalir; firmware bunu logical Linkplay mode olarak `USB` basligiyle gosterir. |
| LP -> MCU | `AXX+PLY+INF{..."mode":"51"...}` | USBDAC varyanti. | Up2Stream kartlarda USB DAC farkli bir MCU/USB yolu olabilir; bu projede fiziksel USB DAC yok. Firmware bu modu desteklenen ayri bir kaynak gibi gostermemeli; USB flash icin esas mode `11`. |
| MCU -> LP | `MCU+MUT+GET` | `AXX+MUT+000` / `001`. | Mute sorgusu. |
| MCU -> LP | `MCU+PLY-PLA` | `AXX+MEA+RDY`. | Play komutu veya oynatma baslatma varyanti. |
| MCU -> LP | `MCU+PLY-STP` | Cevap yok. | Stop current playback. |
| MCU -> LP | `MCU+PLY+PUQ` | `AXX+KEY+NIL`. | Islev belirsiz/NA. |
| MCU -> LP | `MCU+KEY+001` | `AXX+KEY+NIL`. | Muhtemelen uygulanmiyor. |
| LP -> MCU | `AXX+GET+RTC` | Boot sirasinda geliyor. | Firmware su an bunu cevaplamiyor; RTC konusu icin aday. |
| LP -> MCU | `AXX+CHN+GET` | Boot sirasinda geliyor. | Kanal bilgisi sorgusu; cevap davranisi netlestirilecek. |
| LP -> MCU | `AXX+CHN+001/002/000` | Sol/sag/stereo kanal olayi. | App tarafindan gelebiliyor. |
| LP -> MCU | `AXX+PMT+000/001` | Voice prompt basla/bitis veya prompt status. | Boot ve app komutlari sirasinda gorulmus. |
| LP -> MCU | `AXX+SIL+001` | Bluetooth'a gecis istegi gibi not edilmis. | Firmware su an islemiyor. |
| LP -> MCU | `AXX+SIL+000` | Line In'e gecis istegi gibi not edilmis. | Firmware su an islemiyor. |
| LP -> MCU | `AXX+SLV+YES/000/001/NOT` | Multiroom slave/master durumlari. | `001` baska cihaz bize slave oldu: header `M`; `YES` biz slave olduk: header `Slave`; `000/NOT` clear. |
| LP -> MCU | `AXX+BT+GPRL` | Boot sirasinda gorulmus. | BT profili/eslesme listesi olabilir; netlestirilecek. |
| LP -> MCU | `AXX+FACTORY` | Factory/reset surecinde gorulmus. | Firmware su an islemiyor. |
| LP -> MCU | `AXX+DEV+RST` | Restart/factory sonrasi gorulmus. | Firmware su an islemiyor. |
| LP -> MCU | `AXX+STA+001/002` | Network state olaylari. | Firmware boot ekraninda ara mesaj icin kullaniyor; WiFi hazir/yok kararini hala `WWW` belirliyor. |

## 2026-08-24 Debug Bulgulari

Bu bolum klon ST-Link + debug UART ile, `lplog` acikken canli AX31 modul
uzerinden olculen davranislari kaydeder.

### Boot / Ready

| Akis | Gozlem | Sonuc |
| --- | --- | --- |
| Cold boot / power cycle | `AXX+BOT+DON`, `AXX+BOT+UP0`, sonra `AXX+MCU+RDY` goruldu. | `BOT` cold boot/heartbeat alanina ait. |
| Elle `lp MCU+BOT+DON` | LP `AXX+BOT+DON` ile cevap verdi. Firmware otomatik `MCU+BOT+DON` donunce LP bir kez daha `AXX+BOT+DON` verdi. | `AXX+BOT+DON` hem boot bildirimi hem de `MCU+BOT+DON` ACK'i gibi davranabiliyor. Sonsuz dongu flag ile engellenmeli. |
| `AXX+MCU+RDY` | Firmware ilk hazir olusta `MCU+VER+0035&MCU+PLM-000`, `MCU+CAP+PLM00008214&`, volume, neutral EQ ve mevcut EQ raporlar. | Ready sonrasi state raporlama calisiyor; tekrar eden RDY'ler metadata hazir olayi gibi yorumlanmamali. |

### Power

| Komut | Olculen cevap/event | Sonuc |
| --- | --- | --- |
| `MCU+POW+STA` | `AXX+POW+000` | `000` aktif/normal state. |
| `MCU+POW+SAV` | `AXX+POW+001`, sonra `AXX+RA0+000` | Power-save/sleep state. `RA0+000` AP/interface kapanisi olabilir. |
| `MCU+POW+STA` after SAV | `AXX+POW+001` | `001` power-save state dogrulandi. |
| `MCU+POW+RES` | `AXX+STA+000`, `AXX+POW+000`, ardindan `STA/WWW/PMT/MUT` event dalgasi. | Power-save'den aktif moda donus. Tam boot degil; `AXX+BOT...` uretmedi. |
| `MCU+POW+OFF` | Yaklasik 5.16 s sonra `AXX+POW+OFF`. | Hard-off hazirlik handshake'i calisiyor. 2 s timeout kisa; 8-10 s beklemek daha guvenli. |
| `MCU+POW+STA` after OFF without rail cut | `AXX+POW+000` | `POW+OFF` sonrasi MCU rail kesmezse modul aktif state'e donebiliyor veya en azindan aktif raporluyor. |

`MCU+POW+RES` notu: Resume sonrasi "connected to your WiFi network" prompt'u
bir veya iki kez duyulabiliyor. Logda `WWW`, `PMT` ve `MUT` event'leri iki
dalga halinde gelebiliyor. Bu nedenle standby'dan donerken `POW+RES` sadece
`POW+STA` sonucu `001` ise kullanilmali; `000` iken gereksiz RES prompt
yan etkisi yaratabilir.

### RTC / Time

| Akis | Gozlem | Sonuc |
| --- | --- | --- |
| Boot sirasinda | `AXX+GET+RTC` geldi, firmware `MCU+RTC+GET` dondu. | "Gecerli RTC yok, network time ver" yaklasimi calisiyor. |
| Cold boot erken | `MCU+RTC+GET` sonrasi `AXX+RTC+NIL` goruldu. | LP henuz network/NTP time bilmiyorsa NIL donebiliyor. |
| WiFi/NTP sonrasi | `AXX+SET+RTC20260824185015&`, `AXX+SET+WEK1&` ve bazi akislarda `AXX+NXT+ALS-1&` goruldu. | LP network time elde edince MCU'ya push ediyor. `WEK` haftanin gunu; `ALS` daylight-saving/local offset bilgisi gibi gorunuyor. Firmware RTC string'i isliyor; `WEK/ALS` su an saat uzerine ayrica uygulanmiyor. |
| Playback sirasinda | `lp MCU+RTC+GET` test edilecek. | Calarken zaman sorgusu yapilabiliyor mu henuz olculmedi. |

### Channel

| Akis | Gozlem | Sonuc |
| --- | --- | --- |
| Boot sirasinda | `AXX+CHN+GET` geldi. | LP kanal bilgisini soruyor. |
| Firmware cevabi | `MCU+CHN+000` gonderildi. | `000` stereo cevabi kabul ediliyor; akisi bozmadigi gozlemlendi. |

### Spotify / Network Playback

| Akis | Gozlem | Sonuc |
| --- | --- | --- |
| Spotify secimi | `AXX+PLM+031`, `AXX+MEA+RDY`, firmware `MCU+MEA+GET` gonderdi. | `031` Network icinde Spotify servis ipucu. DSP source Network kalir, metadata zinciri baslar. |
| Metadata | `AXX+MEA+DAT{... "vendor":"53706F74696679" ...}&`, sonra `MCU+PINFGET`. | Vendor Spotify; title/artist/album hex gelebiliyor. |
| Player info | `AXX+PLY+INF{..."mode":"31","status":"pause/play",...}` | Player state, duration, position, volume, mute aliniyor. |
| Play start | `AXX+PLY+001` geldi; firmware `AMP_ENABLE ON from lp-ply` ile uyandi. | LP playback auto-wake calisiyor. |
| Bitişik frame | `AXX+SPY+001AXX+MEA+DAT...` goruldu. | Parser `AXX+` ayirimi sayesinde bitisik frame'leri isleyebiliyor. `SPY+001` su an ignore. |

Not: `AXX+MEA+RDY`, AX31 tarafinda media info hazir olayi gibi davranir.
Spotify standby wake denemelerinde `PLM+031` sonrasi zaten `MCU+MEA+GET`
gonderildigi icin, `MEA+RDY` uzerine tekrar `MCU+MEA+GET` basmak akisi
donguye sokabildi. Firmware bu event'te dogrudan `MCU+PINFGET` sorar.

Ornek decode:

- Title: `Kara Bahtima Kustum`
- Artist: `Tuncay Kemertas`
- Album: `Kayla`
- Vendor: `Spotify`

## A98 PDF'ten Ek Olarak Buldugumuz/Aday Komutlar

Bu bolumdeki komutlar eski AX31 modulde mutlaka ayni calisir kabul edilmemeli.
Ancak protokol akisini anlamak ve deneme listesi cikarmak icin onemli.

### Boot ve OOBE

| Yon | Komut | PDF'teki anlam | Proje notu |
| --- | --- | --- | --- |
| LP -> MCU | `AXX+CAP+GET` | Factory/OOBE sirasinda urun capability bilgilerini ister. | Firmware cevapliyor; normal AX31 boot'ta her zaman gelmeyebilir. |
| MCU -> LP | `MCU+CAP+PRJproject_name&` | Zorunlu proje/urun adi. | OOBE gerekiyorsa eklenebilir. |
| MCU -> LP | `MCU+PTV+nnn` | MCU protokol versiyonu. | AX31 firmware `MCU+VER+0035` kullaniyor; fark not edilmeli. |
| MCU -> LP | `MCU+CAP+001xxxxxxxx&` | Capability bitleri. | Bit 16: MCU RTC var. Bizde RTC var ama backup pil yok. |
| MCU -> LP | `MCU+CAP+002xxxxxxxx&` | Capability bitleri 2. | Gercek CAP2 degeri net degil; `INF` icindeki `capability` ile ayni kabul edilmemeli. |
| MCU -> LP | `MCU+CAP+LAUlanguage&` | Varsayilan dil. | Gerekmeyebilir. |
| MCU -> LP | `MCU+CAP+STMxxxxxxxx&` | Stream servislerini enable/disable eder. | App tarafini sade tutmak icin ileride yararli. |
| MCU -> LP | `MCU+CAP+PLMxxxxxxxx&` | Desteklenen local input mode'lar. | Test edildi: `00008016` WiFi/BT/USB/Aux/Optical, `00008214` WiFi/BT/USB/XLR/Optical. Firmware hedefi `00008214`; bu projede USB DAC fiziksel olarak yok, USB flash playback icin `PLY+INF mode=11` esas alinir. |
| MCU -> LP | `MCU+SID+name&` | Default SSID, CAP cevaplarinin sonunda. | Firmware `DSP_Amp` donuyor. |
| LP -> MCU | `AXX+BOT+DON` | Modul boot ediyor ama hazir degil. | AX31 RE ile uyumlu. |
| LP -> MCU | `AXX+BOT+UP0` | Modul acildi; `MCU+BOT+DON` heartbeat/boot cevabi sonrasi da gelir. | AX31 RE ile uyumlu. |
| LP -> MCU | `AXX+MCU+RDY` | Modul MCU ile konusmaya hazir. | Firmware isliyor. |

### Power, Sleep ve Firmware Update

| Yon | Komut | PDF'teki anlam | Proje notu |
| --- | --- | --- | --- |
| MCU -> LP | `MCU+POW+OFF` | WiFi module power-off hazirligi ister. | AX31'de test edildi; `AXX+POW+OFF` yaklasik 5.16 s sonra geldi. |
| LP -> MCU | `AXX+POW+OFF` | Kapatma izni/hazir cevabi. | Gelirse LP rail kesilebilir; 8-10 s timeout onerilir. |
| LP -> MCU | `AXX+BURNING` | OTA/update devam ediyor; guc kesilmemeli. | Hard-off state machine'e mutlaka eklenmeli. |
| LP -> MCU | `AXX+BURN002` | Update/lighting etkileri icin varyant olarak geciyor. | Sadece not. |
| MCU -> LP | `MCU+POW+SAV` | Sleep/power-save. | AX31'de `AXX+POW+001` + `AXX+RA0+000` ile goruldu. |
| MCU -> LP | `MCU+POW+RES` | Sleep'ten uyandirma. | AX31'de test edildi; `AXX+POW+000` ve network/prompt event dalgasi uretir. |
| MCU -> LP | `MCU+POW+STA` | Power status sorgusu. | AX31'de test edildi: `000` active, `001` power-save. |
| MCU -> LP | `MCU+FCRYPOW` | Factory restore ve power-off istegi. | Riskli; yalniz test modunda. |
| LP -> MCU | `AXX+DEV+RST` | Modul restart bildirimi. | Restart durumunda UI/source state temizlenebilir. |

### RTC, Zaman ve Alarm

| Yon | Komut | PDF'teki anlam | Proje notu |
| --- | --- | --- | --- |
| LP -> MCU | `AXX+GET+RTC` | MCU RTC bilgisini ister. | Firmware `MCU+RTC+GET` ile network time ister. |
| LP -> MCU | `AXX+SET+RTCYYYYMMDDHHMMSS&` | WiFi/app MCU saatini set eder. | Firmware su an isliyor. |
| LP -> MCU | `AXX+APP+RTCYYYYMMDDHHMMSS&` | App tarafindan gelen RTC set. | Firmware su an islemiyor; `SET+RTC` ile benzer ele alinabilir. |
| MCU -> LP | `MCU+RTC+GET` | MCU WiFi'dan saat ister. | Firmware bunu `AXX+GET+RTC` icin "gecerli RTC yok, network time ver" cevabi olarak kullaniyor. |
| MCU -> LP | `MCU+SND+RTCYYYYMMDDHHMMSS&` | MCU WiFi RTC'sini set eder. | AX31'de etkili oldugu goruldu, fakat `MCU+RTC+GET` ile geri soruldugunda verilen saate yaklasik +6 saat uygulanmis deger donuyor. Otomatik firmware akisinda kullanilmamali. |
| MCU -> LP | `MCU+WEK+GET` | Alarm/wek bilgisi sorgusu. | Bu projede oncelik dusuk. |

RTC karari: STM32 RTC peripheral LSE ile calisiyor ve VBAT tarafinda harici pil
beslemesi var. Firmware live clock icin donanim RTC'yi tercih ediyor; harici
25xx EEPROM sadece snapshot/fallback olarak kullaniliyor. LP internete bagliysa
`MCU+RTC+GET` ile gelen `AXX+SET+RTC...` guvenilir NTP kaynagi kabul ediliyor.
MCU'dan LP'ye saat set etme tarafi (`MCU+SND+RTC...`) AX31'de offsetli
davrandigi icin normal akisa alinmadi.

### Bluetooth ve Kaynak Gecisleri

| Yon | Komut | PDF'teki anlam | Proje notu |
| --- | --- | --- | --- |
| LP -> MCU | `AXX+BT+CON1` | Bluetooth baglandi. | Firmware su an ayrica BT UART izliyor; LP event'i desteklenebilir. |
| LP -> MCU | `AXX+BT+CON0` | Bluetooth baglanti koptu. | UI/source state icin aday. |
| MCU -> LP | `MCU+BT+CONN` | Son BT cihaza baglanma istegi. | Bizde BT module ayri UART'ta; LP'ye gerek olup olmadigi test edilmeli. |
| MCU -> LP | `MCU+BT+PAIR` | BT pairing moduna gecme istegi. | Bizde BT module ayri UART'ta; sadece LP app senkronu icin aday. |
| LP -> MCU | `AXX+SIL+001` | AX31 RE notuna gore BT'ye gecis istegi olabilir. | A98 PDF'teki BT bolumuyle birlikte tekrar test edilmeli. |

BT UART notu: `+TITL/+ARTS/+ALBM` ve `BT_PA` satirlari telefon ekrani acilip
kapaninca da gelebiliyor. Firmware bu satirlari hemen source takeover sebebi
yapmaz; once kisa bir BT takeover aday penceresi acar. Bu pencerede `+SRC=NONE`
gelirse aday iptal edilir, gelmezse LP'ye `MCU+PLY+PUS` atilip source Bluetooth'a
alinir. BT'ye dogrudan gecis menu/IR veya LP app'ten gelen `AXX+PLM+041`
uzerinden de yapilir.
| LP -> MCU | `AXX+SIL+000` | AX31 RE notuna gore Line In'e gecis istegi olabilir. | Tekrar test edilmeli. |

### App / Pass-through / EQ

| Yon | Komut | PDF'teki anlam | Proje notu |
| --- | --- | --- | --- |
| LP -> MCU | `AXX+PAS+data&` | App'ten MCU'ya pass-through veri. | Firmware EQ icin kullaniyor. |
| MCU -> LP/App | `MCU+PAS+data&` | MCU'dan app'e pass-through veri. | Firmware EQ report icin kullaniyor. |
| LP -> MCU | `AXX+PAS+EQGet&` | App EQ durumunu sorar. | Firmware cevapliyor. |
| LP -> MCU | `AXX+PAS+EQSet:bass:n&` | Bass EQ set. | Firmware DSP bass gain'e ceviriyor. |
| LP -> MCU | `AXX+PAS+EQSet:treble:n&` | Treble EQ set. | Firmware DSP treble gain'e ceviriyor. |
| MCU -> LP/App | `MCU+PAS+EQ:bass:n&` / `MCU+PAS+EQ:treble:n&` | PDF'te MCU cevabi olarak geciyor. | Firmware su an `EQSet` formuyla raporluyor; ileride uyumluluk icin karsilastirilmali. |

## Firmware Icin Karar / Deneme Listesi

1. Hard-off state machine tasarlanabilir.
   - `MCU+POW+OFF` gonder.
   - `AXX+POW+OFF` icin 8 s bekle.
   - `AXX+POW+OFF` gelirse mute LED 2 kisa blink yapar; kullanici fisi cekebilir.
   - `AXX+BURNING` gelirse guc kesme iptal edilmeli.
   - `AXX+BURNING` gelirse mute LED 3 hizli blink yapar ve ON kalir.
   - Timeout durumunda mute LED 4 blink yapar ve ON kalir.
   - Mevcut PCB'de LP rail kontrol IO'su ayrilmadigi icin hard-off fiziksel
     guc kesmez; guvenli fis cekme geri bildirimi verir.

2. Standby/power-save politikasi ayrilmali.
   - `MCU+POW+SAV`: LP power-save; `AXX+POW+001` ve `AXX+RA0+000`.
   - `MCU+POW+RES`: LP active; `AXX+POW+000`, ama WiFi prompt/event dalgasi yaratabilir.
   - Standby'dan donerken once `MCU+POW+STA` sormak, `000` ise `RES` gondermemek daha iyi olabilir.

3. Boot handshake gozlemi tamamlandi ama urun davranisi netlestirilmeli.
   - `AXX+BOT+DON/UP0` cold boot alaninda goruldu.
   - Firmware bir kez `MCU+BOT+DON` donuyor.
   - Elle `MCU+BOT+DON` gondermek `AXX+BOT+DON` ACK'i uretir; otomatik cevap flag ile sinirlanmali.
   - Hard-off sonrasi cold boot ekraninda hedef akış:
     `Handshake ready` -> `WiFi Connected` -> `Time synced` -> Standby ekran.
   - "Connected to your WiFi network" anonsu handshake sonuna yakin geldigi icin
     ekranda handshake'ten sonra ayri bir `WiFi Connected` asamasi gosterilmeli.
   - `Time synced` son asama olmali; alttaki ilerleyen nokta animasyonu bu
     asamada son noktaya gelmis gorunmeli.

4. RTC davranisi bugun icin yeterli.
   - `AXX+GET+RTC` -> `MCU+RTC+GET`.
   - LP hazir degilse `AXX+RTC+NIL`.
   - Network time gelince `AXX+SET+RTC...` push ediyor.
   - `AXX+APP+RTC...` ve `AXX+SET+WEK...` ileride sessiz parser destegine alinabilir.

5. BT source gecisleri icin PDF komutlari test edilmeli.
   - `AXX+BT+CON1/CON0`
   - `MCU+BT+CONN`
   - `MCU+BT+PAIR`
   - AX31'de gorulen `AXX+BT+GPRL`, `AXX+SIL+001`, `AXX+SIL+000`

6. `lp MCU+RTC+GET` playback sirasinda test edilmeli.
   - Calarken `AXX+SET+RTC...`, `AXX+RTC+NIL` veya baska cevap geliyor mu kaydedilecek.

7. `AXX+CAP+GET` eski AX31 firmware'de hic geliyor mu bakilmali.
   - Gelirse minimum cevap seti: `MCU+CAP+PRJ...&`, `MCU+PTV...`,
     `MCU+CAP+001...&`, `MCU+CAP+002...&`, `MCU+CAP+PLM...&`, `MCU+SID...&`.
   - Bu kisim OOBE/factory restore akisinda kritik olabilir.

## Hemen Dokunulmamasi Gerekenler

- UART baud/format ayari: Mevcut firmware'deki ayar bu AX31 modul icin dogru
  kabul edilecek.
- `MCU+PLM-xxx` formu: Firmware ve AX31 boot notlari bu formu kullaniyor.
- `MCU+POW+SAV`: Calisan eski davranis olarak korunmali; sadece anlami daha
  dogru etiketlenmeli.
