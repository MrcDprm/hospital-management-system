# Hastane Yönetimi

[English](README.md) | **Türkçe**

Bir poliklinik ya da küçük hastane için C++ ve Qt ile yazılmış masaüstü yönetim uygulaması: hastalar, doktorlar ve bölümler, çakışmasız randevular, muayeneler, reçeteler ve raporlar; rollere göre giriş. Veriler yerelde SQLite'ta tutulur.

> 🚧 Geliştirme sürüyor. Bu README proje planıdır, v1.0.0'da tamamlanacak.

## Plan

### MVP
- **Rollerle giriş:** yönetici, sekreter ve doktor. Şifreler **Argon2id** (libsodium) ile hash'lenir, güçlü şifre kuralları ve yanlış denemelerde artan bekleme vardır. Yetkiler sadece arayüzde değil veri katmanında da kontrol edilir. İlk açılışta yönetici hesabı oluşturulur.
- **Hastalar:** sağlama basamaklı T.C. kimlik numarası, iletişim bilgileri, doğum tarihi, cinsiyet, kan grubu, alerjiler ve kronik hastalıklar. Arama ve CSV'ye aktarma.
- **Bölümler ve doktorlar:** bölümler (kardiyoloji, çocuk sağlığı…), unvanı, bölümü, çalışma günleri ve saatleri, randevu süresiyle doktorlar.
- **Randevular:** bölüm ve doktor seçilir, sadece boş saatler listelenir; çakışan randevu verilemez. Durumlar: alındı, geldi, muayene edildi, gelmedi, iptal.
- **Bugün ekranı:** doktorlara göre bugünün randevuları ve hızlı işlemler (geldi, muayeneye başla, gelmedi).
- **Muayeneler:** doktorun yazdığı şikâyet, bulgular, tanı (ICD-10 kodu ve adı) ve notlar.
- **Masaüstü uygulaması:** koyu ve açık tema, Türkçe ve İngilizce, ikon, sürüm, Hakkında penceresi, veriler kullanıcı klasöründe, Windows kurulum dosyası.
- **Testler:** boş saat hesabı, çakışma kuralları, kimlik doğrulama, yetkiler ve şifre kuralları (Qt Test).

### Ekler
- **Reçeteler:** doz ve kullanım şekliyle ilaçlar, A4 PDF olarak basılır.
- **Haftalık takvim:** her doktorun haftası, dolu ve boş saatleriyle; tıklayınca randevu verilir.
- **Hasta geçmişi:** bir hastanın bütün muayeneleri, tanıları ve reçeteleri tek zaman çizelgesinde.
- **Raporlar:** bölümlere göre randevu sayısı, gelmeme oranı, doktor doluluğu; grafikler ve CSV.
- **Örnek veri:** ilk açılışta bir kez sorulur (bölümler, doktorlar, hastalar ve birkaç aylık randevu).

### Gelecek Planları
- SMS ya da e-postayla randevu hatırlatma.
- Tahlil sonuçları ve dosya ekleri.
- Faturalama ve sigorta.
- Birden fazla şube.

## Kullanılan Teknolojiler
- C++, Qt (Widgets, Sql, Test)
- SQLite, libsodium (Argon2id)
- CMake, MinGW-w64 (MSYS2 UCRT64)
- windeployqt, Inno Setup
