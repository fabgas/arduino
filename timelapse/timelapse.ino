/*
  Timelapse avec ESP32-CAM. Les photos s'enregistrent sur une carte SD.
  On démarre la prise de vue et on contrôle le délai entre 2 images

  Plus d'infos:
  https://electroniqueamateur.blogspot.com/2020/03/time-lapse-avec-lesp32-cam.html

*/

// inclusion des bibliothèques utile
#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"
#include "FS.h"       // manipulation de fichiers
#include "SD_MMC.h"   // carte SD

// paramètres de votre réseau WiFi
const char* ssid = "xxxxxx";
const char* password = "xxxxxxx";

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

int numero_port; // numéro du port du stream server

bool carte_presente = 0; // présence d'une carte micro SD
int numero_fichier = 10000; // numéro initial de la photo

unsigned long photoPrecedente = 0; // prise de la dernière photo
long intervalle = 10000;  // nombre de millisecondes entre 2 photos.
bool timeLapseActif = 0; // devient vrai pendant l'enregistrement de la séquence

// ********************************************************
// stream_handler: routine qui gère le streaming vidéo

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  static int64_t last_frame = 0;
  if (!last_frame) {
    last_frame = esp_timer_get_time();
  }

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Echec de la capture de camera");
      res = ESP_FAIL;
    } else {
      if (fb->width > 400) {
        if (fb->format != PIXFORMAT_JPEG) {
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if (!jpeg_converted) {
            Serial.println("Echec de la compression JPEG");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if (res == ESP_OK) {
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if (res != ESP_OK) {
      break;
    }
  }
  last_frame = 0;

  return res;
}



// ********************************************************
// web_handler: affichage de la page web

static esp_err_t web_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "identity");
  sensor_t * s = esp_camera_sensor_get();

  char pageWeb[450] = "";
  strcat(pageWeb, "<!doctype html> <html> <head> <title id='title'>ESP32-CAM</title> </head> <body> <img id='stream' src='http://");
  // l'adresse du stream server (exemple: 192.168.0.145:81):
  char adresse[20] = "";
  sprintf (adresse, "%d.%d.%d.%d:%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3], numero_port);
  strcat(pageWeb, adresse);
  strcat(pageWeb, "/stream'> <br> <br>");


  if (carte_presente) {
    // on ajoute le bouton
    if (timeLapseActif) {
      strcat(pageWeb, "<form action='/clic' method='GET'> <label for='intervalle'> Intervalle (en secondes):</label> <br> <input type='text' id='intervalle' name='intervalle' value = '10'><br><br><INPUT type='submit' value='Arr&ecirc;ter'></form> ");
    }
    else {
      strcat(pageWeb, "<form action='/clic' method='GET'> <label for='intervalle'> Intervalle (en secondes):</label> <br> <input type='text' id='intervalle' name='intervalle' value = '10'><br><br><INPUT type='submit' value='D&eacute;marrer'></form> ");
    }

  }
  else {
    // on indique l'absence de la carte
    strcat(pageWeb, "<p> Pas de carte microSD, allez au super U.</p>");
  }

  strcat(pageWeb, "</body> </html>");

  int taillePage = strlen(pageWeb);

  return httpd_resp_send(req, (const char *)pageWeb, taillePage);
}

// ********************************************************
/* enregistrer_photo: prise de la photo et création du fichier jpeg  */

void enregistrer_photo (void)
{
  char adresse[20] = ""; // chemin d'accès du fichier .jpeg
  camera_fb_t * fb = NULL; // frame buffer

  // prise de la photo
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Echec de la prise de photo.");
    return;
  }

  numero_fichier = numero_fichier + 1;

  // enregitrement du fichier sur la carte SD

  sprintf (adresse, "/photo%d.jpg", numero_fichier);
  Serial.printf("Affichage de la photo %s",adresse);
  fs::FS &fs = SD_MMC;
  File file = fs.open(adresse, FILE_WRITE);

  if (!file) {
    Serial.println("Echec lors de la creation du fichier.");
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Fichier enregistre: %s\n", adresse);
  }
  file.close();
  esp_camera_fb_return(fb);
}

// ********************************************************
/* clic_handler: Gestion du clic sur le bouton */

static esp_err_t clic_handler(httpd_req_t *req) {


  // récupérons le contenu du champ texte (temps entre 2 poses successives)
  char*  buf;
  size_t buf_len;
  char valeur[32] = {0,};


  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char*)malloc(buf_len);
    if (!buf) {
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }

    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "intervalle", valeur, sizeof(valeur)) == ESP_OK ) {
      } else {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    } else {
      free(buf);
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }

    free(buf);

  } else {
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  intervalle = atoi(valeur) * 1000;

  timeLapseActif = !(timeLapseActif);

  return (web_handler(req));
}


// ********************************************************
// startCameraServer: démarrage du web server et du stream server

void startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = web_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t clic_uri = {
    .uri       = "/clic",
    .method    = HTTP_GET,
    .handler   = clic_handler,
    .user_ctx  = NULL
  };

  Serial.printf("Demarrage du web server sur le port: '%d'\n", config.server_port);
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &clic_uri);
  }

  config.server_port += 1;
  config.ctrl_port += 1;
  Serial.printf("Demarrage du stream server sur le port: '%d'\n", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }

  numero_port = config.server_port;
}

// ********************************************************
// initialisation de la caméra, connexion au réseau WiFi.

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("====");

  // définition des broches pour le modèle AI Thinker - ESP32-CAM
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;  //YUV422|GRAYSCALE|RGB565|JPEG
  config.frame_size = FRAMESIZE_UXGA;  // QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  config.jpeg_quality = 10;  // 0-63 ; plus bas = meilleure qualité
  config.fb_count = 2; // nombre de frame buffers

  // initialisation de la carte micro SD

  if (SD_MMC.begin()) {
    uint8_t cardType = SD_MMC.cardType();
    if (cardType != CARD_NONE) {
      carte_presente = 1;
    }
  }

  if (!carte_presente) {
    Serial.println("Pas de carte microSD dans l'ESP32-CAM");
  }

  // initialisation de la caméra
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Echec de l'initialisation de la camera, erreur 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();

  Serial.println("");
  Serial.print("Connexion au reseau WiFi: ");
  Serial.println(ssid);
  delay(100);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
  }

  Serial.println("WiFi connecte");
  Serial.println("");

  delay(100);

  startCameraServer();

  Serial.print("La camera est prete.  Utilisez 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' pour vous connecter.");

}

// ********************************************************

void loop() {

  // enregistrement d'une photo, si c'est le bon moment

  unsigned long maintenant = millis();
  if (maintenant - photoPrecedente >= intervalle) {
    photoPrecedente = maintenant;
    if (timeLapseActif) {
      enregistrer_photo();
    }
  }
}
