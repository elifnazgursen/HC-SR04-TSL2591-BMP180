/*
 * TSL2591.c
 *
 *  Created on: Mar 17, 2026
 *      Author: elifnazgursen
 */


#include "TSL2591.h"

extern I2C_HandleTypeDef hi2c1;


	#define ADDR (0x29 << 1) // sensörün i2c 7 bit adresi (0x29), 1 kere sola kayd. çünk son bit read/write kullanılır gnld. biz HAL e kaydırarak veririz
	#define CMD_BIT 0xA0 // sensöre register erişimi yaparken eklenen özel komut biti

	#define ENABLE_REG   0x00 // açıl çalış ölçüme başla
	#define CONFIG_REG   0x01 // config reg i, ne kadar hassas ölçsün ya da ne kadar ölçsün
	#define STATUS_REG   0x13 // status - veri hazır mı? ölçüm olmuş mu?

	#define C0DATAL_REG  0x14 // channel 0 ın alt byte ı
	#define C1DATAL_REG  0x16 // channel 1 in alt byte ı


		//FONK. GÖREVİ: SENSÖR İÇİNDEKİ BİR REG E VERİ YAZMAK
		HAL_StatusTypeDef WriteReg(uint8_t reg, uint8_t data) // hangi reg e ne yazacağız (data) ?
		{
			uint8_t buf[2];

			buf[0] = CMD_BIT | reg; // gidilecek register (Mesela reg = 0x00 ise: 0xA0 | 0x00 = 0xA0 ben ENABLE REG e gidiyorum demek)

			buf[1] = data; // yazılacak komut-veri

			// I2C ÜZERİNDEN VERİ YOLLAMA
			// (hangi i2c?,kiminle konuşacağız?, hangi veriyi göndereceğiz?, kaç byte göndericez?, ne kadar bekliyim?)

			return HAL_I2C_Master_Transmit(&hi2c1, ADDR, buf, 2, HAL_MAX_DELAY); // neden return? -> HAL fonksiyonundan gelen sonucu aynen geri gönderiyoruz
		}




		//FONK. GÖREVİ: SENSÖRDEN 1 BYTE VERİ OKUMAK
		HAL_StatusTypeDef ReadReg(uint8_t reg, uint8_t *data) // hangi reg okunsun? okunan veri nereye yazılsın?
		{
		    uint8_t cmd = CMD_BIT | reg; // hangi reg i okumak istediğimi söylüyorum, önce red adr gönderiyoruz

		    if (HAL_I2C_Master_Transmit(&hi2c1, ADDR, &cmd, 1, HAL_MAX_DELAY) != HAL_OK)
		        return HAL_ERROR; // ilk gönderim başarısızsa veri okumaya devam etme


		    // veriyi okuyan satır ( i2c üzerinden karşı taraftan veri alır)
		    // (i2c1 kullan, sensörden al, aldığın veriyi dataya koy, 1 byte al, bekle)

		    return HAL_I2C_Master_Receive(&hi2c1,ADDR, data, 1, HAL_MAX_DELAY);
		}




		//FONK. GÖREVİ: sensörden 2 byte okuyup bunu 16 bit sayı haline getirmek
		HAL_StatusTypeDef Read2Bytes(uint8_t startReg, uint16_t *value)
		{
		    uint8_t cmd = CMD_BIT | startReg;
		    uint8_t buf[2]; // 2 byte geçici olarak burada saklanır

		    // şu register’dan okumaya başlayacağım
		    if (HAL_I2C_Master_Transmit(&hi2c1, ADDR, &cmd, 1, HAL_MAX_DELAY) != HAL_OK)
		        return HAL_ERROR;

		    // Sonra sensörden 2 byte alıyoruz.
		    if (HAL_I2C_Master_Receive(&hi2c1, ADDR, buf, 2, HAL_MAX_DELAY) != HAL_OK)
		        return HAL_ERROR;

		    *value = (uint16_t)(buf[1] << 8) | buf[0]; // Burada iki ayrı byte’ı birleştirip tek sayı yapıyoruz.
		    return HAL_OK;
		}



		//FONK. GÖREVİ: SENSÖRÜ BAŞLATMAK. sensör doğru mu, bağlı mı, çalışmaya hazır mı?
		// sensörü başlat -> sonuç döndür
		HAL_StatusTypeDef TSL2591_Init(void)
		{
		    uint8_t id = 0; // sensör kimlik bilgisi

		    if (ReadReg(0x12, &id) != HAL_OK) // sensörün id reg ini okuyoruz datasheetten. gerçekten tsl mi?
		        return HAL_ERROR; // eğer okunmuyorsa dur

		    if (id != 0x50)  //okuduğum kimlik 0x50 değilse bu beklediğim sensör değil
		        return HAL_ERROR;

		    if (WriteReg(ENABLE_REG, 0x03) != HAL_OK) // SENSÖRÜ AÇIYORUZ ENABLE_REG = 0x00 Bu register’a 0x03 yazıyoruz. power on
		        return HAL_ERROR;

		    if (WriteReg(CONFIG_REG, 0x10) != HAL_OK) // Burada ayar yapıyoruz. 0x10 ile örnek bir gain/integration ayarı veriyoruz.
		        return HAL_ERROR;

		    HAL_Delay(120);

		    return HAL_OK; //Her şey yolunda gittiyse: sensör bulundu - sensör açıldı -  ayar yapıldı - o zaman başarıyla çık.
		}




		//FONK. GÖREVİ: sensörden gerçek ışık verilerini almak
		HAL_StatusTypeDef ReadRaw(uint16_t *ch0, uint16_t *ch1)
		{
		    uint8_t status = 0;

		    if (ReadReg(STATUS_REG, &status) != HAL_OK)  // veri hazır mı? sensör ölçümü bitirmiş mi?
		        return HAL_ERROR;

		    if ((status & 0x01) == 0) // Datasheette AVALID biti veri geçerli mi sorusunu cevaplar. 0 ise demek ki veri henüz hazır değil.
		        return HAL_BUSY; // “bekle, daha ölçüm bitmedi”

		    if (Read2Bytes(C0DATAL_REG, ch0) != HAL_OK) //görünür + IR karışımı
		        return HAL_ERROR;

		    if (Read2Bytes(C1DATAL_REG, ch1) != HAL_OK) //daha çok IR
		        return HAL_ERROR;

		    return HAL_OK;
		}
