// --- file: SV5W.h
#pragma once
#include <Arduino.h>
#include <vector>

// Lightweight UART driver for DY-SV5W voice module (UART mode)
// Protocol per datasheet: Start=0xAA, CMD, LEN, DATA[LEN], CHECKSUM(low 8 bits of sum)
// Baud: 9600, 8N1

class SV5W {
public:
  enum class Command : uint8_t {
    // Control commands
    PLAY              = 0x02, // AA 02 00 AC
    PAUSE             = 0x03, // AA 03 00 AD
    STOP              = 0x04, // AA 04 00 AE
    PREVIOUS          = 0x05, // AA 05 00 AF
    NEXT              = 0x06, // AA 06 00 B0
    VOL_UP            = 0x14, // AA 14 00 BE
    VOL_DOWN          = 0x15, // AA 15 00 BF
    PREVIOUS_FILE     = 0x0E, // AA 0E 00 B8
    NEXT_FILE         = 0x0F, // AA 0F 00 B9
    STOP_PLAYING      = 0x10, // AA 10 00 BA (alias of STOP on some firmwares)

    // Query commands
    QUERY_PLAY_STATUS           = 0x01, // returns AA 01 01 <status> SM
    QUERY_CURRENT_ONLINE_DRIVE  = 0x09, // returns AA 09 01 <drive> SM
    QUERY_CURRENT_PLAY_DRIVE    = 0x00, // returns AA 00 01 <drive> SM
    QUERY_NUMBER_OF_SONGS       = 0x0C, // returns AA 0C 02 <hi> <lo> SM
    QUERY_CURRENT_SONG          = 0x0D, // returns AA 0D 02 <hi> <lo> SM
    QUERY_FOLDER_DIR_SONG       = 0x11, // returns AA 11 02 <hi> <lo> SM
    QUERY_FOLDER_SONG_COUNT     = 0x12, // returns AA 12 02 <hi> <lo> SM
    QUERY_VERSION               = 0x08, // returns AA 08 <len> <ver...> SM

    // Setting commands
    SET_VOLUME          = 0x13, // AA 13 01 <vol> SM (0..255, typical 0..30)
    SET_LOOP_MODE       = 0x18, // AA 18 01 <mode> SM (0..7)
    SET_CYCLE_TIMES     = 0x19, // AA 19 02 <H> <L> SM
    SET_EQ              = 0x1A, // AA 1A 01 <eq> SM (0..4)
    SPECIFIED_SONG      = 0x07, // AA 07 02 <idxH> <idxL> SM (play by index)
    SPECIFIED_PATH      = 0x0A, // AA 0A <len> <drive> <path...> SM
    SWITCH_SPECIFIED_DRIVE = 0x0B, // AA 0B 01 <drive> SM
    SPECIFIED_SONG_INTERPLAY = 0x16, // AA 16 03 <drive> <idxH> <idxL> SM
    SPECIFIED_PATH_INTERPLAY = 0x17, // AA 17 <len> <drive> <path...> SM
    SELECT_NO_PLAY      = 0x1F  // AA 1F 02 <idxH> <idxL> SM
  };

  struct Response {
    uint8_t cmd = 0;      // echoed command
    std::vector<uint8_t> data; // payload
    bool valid = false;   // checksum ok and length matches
  };

  // Initialize with a HardwareSerial (e.g., Serial2), RX/TX pins and baud (default 9600)
  void begin(HardwareSerial& port, int rxPin, int txPin, uint32_t baud = 9600, uint32_t respTimeoutMs = 50) {
    serial_ = &port;
    serial_->begin(baud, SERIAL_8N1, rxPin, txPin);
    timeoutMs_ = respTimeoutMs;
  }

  // --- High-level helpers ---
  void play()                     { sendSimple(Command::PLAY); }
  void pause()                    { sendSimple(Command::PAUSE); }
  void stop()                     { sendSimple(Command::STOP); }
  void next()                     { sendSimple(Command::NEXT); }
  void previous()                 { sendSimple(Command::PREVIOUS); }
  void nextFile()                 { sendSimple(Command::NEXT_FILE); }
  void previousFile()             { sendSimple(Command::PREVIOUS_FILE); }
  void stopPlaying()              { sendSimple(Command::STOP_PLAYING); }

  // Volume en EQ
  void setVolume(uint8_t vol /*0..255; typically 0..30*/) {
    if(vol>30) vol=30; // clamp to typical max
    sendWithData(Command::SET_VOLUME, &vol, 1);
  }

  void increaseVolume()          { sendSimple(Command::VOL_UP); }
  void decreaseVolume()          { sendSimple(Command::VOL_DOWN); }

  void setEQ(uint8_t eq /*0..4*/) {
    if(eq>4) eq=4; // clamp
    sendWithData(Command::SET_EQ, &eq, 1);
  }

  void setLoopMode(uint8_t mode /*0..7 per datasheet*/) {
    if(mode>7) mode=7; // clamp
    sendWithData(Command::SET_LOOP_MODE, &mode, 1);
  }

  void setCycleTimes(uint16_t times) {
    uint8_t d[2] = { (uint8_t)(times >> 8), (uint8_t)times };
    sendWithData(Command::SET_CYCLE_TIMES, d, 2);
  }

  // Play a specific song index (1..65535) per datasheet numbering
  void playTrack(uint16_t index) { 
    if(index == 0) index = 1; // clamp to min
    else if(index > 65535) index = 65535; // clamp to max
    uint8_t d[2] = { (uint8_t)(index >> 8), (uint8_t)index };
    sendWithData(Command::SPECIFIED_SONG, d, 2);
  }

  // Select a song by index but do not start playback
  void selectTrackNoPlay(uint16_t index) {
    uint8_t d[2] = { (uint8_t)(index >> 8), (uint8_t)index };
    sendWithData(Command::SELECT_NO_PLAY, d, 2);
  }

  // Optionally select drive: 0x00=USB, 0x01=SD, 0x02=FLASH (per datasheet; check table)
  void switchDrive(uint8_t driveCode) {
    sendWithData(Command::SWITCH_SPECIFIED_DRIVE, &driveCode, 1);
    defaultDrive_ = driveCode;
  }

  // Play by filename/path: drive (0x00 USB, 0x01 SD, 0x02 FLASH), path like "\folder\00001.mp3"
  void playByPath(uint8_t drive, const char *path)
  {
    if (!path)
      return;
    uint8_t plen = (uint8_t)strnlen(path, 250); // limit
    // LEN = 1 (drive) + N (path)
    uint8_t len = (uint8_t)(1 + plen);
    uint8_t *buf = (uint8_t *)alloca(len);
    buf[0] = drive;
    memcpy(buf + 1, path, plen);
    sendWithData(Command::SPECIFIED_PATH, buf, len);
  }
  // Overload: gebruik standaard drive die we bijhouden (default 0x01=SD)
  void playByPath(const char* path) { playByPath(defaultDrive_, path); }
  void playByPath(const String& path) { playByPath(defaultDrive_, path.c_str()); }
  // Default drive beheren
  void setDefaultDrive(uint8_t drive) { defaultDrive_ = drive; }
  uint8_t defaultDrive() const { return defaultDrive_; }

  // Speel een willekeurig bestand uit een map met numerieke bestandsnamen (bv. \FOLDER\00001.MP3 .. \FOLDER\00020.MP3)
  void playRandomFolderClip(const char *folder,
                            uint16_t maxIndex,
                            uint16_t minIndex = 1,
                            const char *ext = "MP3")
  {
    if (!folder || !ext)
      return;
    if (maxIndex < minIndex)
      return;

    // Kies random index binnen [minIndex, maxIndex]
    uint16_t span = (uint16_t)(maxIndex - minIndex + 1);
    uint16_t idx = (uint16_t)(minIndex + (esp_random() % span));

    // Bestandsnaam: 00001.MP3
    char fname[16];
    snprintf(fname, sizeof(fname), "%05u.%s", (unsigned)idx, ext);

    // Pad opbouwen: zorg voor exact één leading backslash en geen dubbele scheidingstekens
    // Resultaat: \FOLDER\00001.MP3
    char path[256];
    size_t pos = 0;

    // Leading backslash
    path[pos++] = '\\';

    // Kopieer folder zonder leading/trailing slashes
    const char *f = folder;
    while (*f == '\\' || *f == '/')
      ++f; // strip leading
    size_t flen = strnlen(f, 200);
    while (flen > 0 && (f[flen - 1] == '\\' || f[flen - 1] == '/'))
      --flen; // strip trailing

    // Voeg folder toe
    for (size_t i = 0; i < flen && pos < sizeof(path) - 1; ++i)
    {
      char c = f[i];
      if (c == '/')
        c = '\\';
      path[pos++] = c;
    }

    // Scheidingsteken tussen folder en bestandsnaam
    if (pos < sizeof(path) - 1)
      path[pos++] = '\\';

    // Voeg bestandsnaam toe
    for (size_t i = 0; i < sizeof(fname) - 1 && fname[i] && pos < sizeof(path) - 1; ++i)
    {
      path[pos++] = fname[i];
    }
    path[pos] = '\0';

    // Afspelen via default drive (of gebruik playByPath(drive, path) voor een specifieke drive)
    playByPath(path);
  }

  // --- Queries (read back a short response) ---
  Response queryPlayStatus()            { return sendAndRead(Command::QUERY_PLAY_STATUS); }
  Response queryCurrentOnlineDrive()    { return sendAndRead(Command::QUERY_CURRENT_ONLINE_DRIVE); }
  Response queryCurrentPlayDrive()      { return sendAndRead(Command::QUERY_CURRENT_PLAY_DRIVE); }
  Response queryNumberOfSongs()         { return sendAndRead(Command::QUERY_NUMBER_OF_SONGS); }
  Response queryCurrentSong()           { return sendAndRead(Command::QUERY_CURRENT_SONG); }
  Response queryFolderDirSong()         { return sendAndRead(Command::QUERY_FOLDER_DIR_SONG); }
  Response queryFolderSongCount()       { return sendAndRead(Command::QUERY_FOLDER_SONG_COUNT); }
  Response queryVersion() { return sendAndRead(Command::QUERY_VERSION); }

  // Low-level: send arbitrary command with a data buffer
  void sendWithData(Command cmd, const uint8_t* data, uint8_t len) {
    if (!serial_) return;
    const uint8_t start = 0xAA; //Start byte SV5W command
    const uint8_t c = (uint8_t)cmd;
    uint8_t checksum = start + c + len;
    serial_->write(start);
    serial_->write(c);
    serial_->write(len);
    for (uint8_t i = 0; i < len; ++i) {
      checksum += data[i];
      serial_->write(data[i]);
    }
    checksum &= 0xFF; // low 8 bits
    serial_->write(checksum);
  }

  // Low-level: send a no-data command (LEN=0)
  void sendSimple(Command cmd) {
    if (!serial_) return;
    const uint8_t start = 0xAA; //Start byte SV5W command
    const uint8_t c = (uint8_t)cmd;
    uint8_t checksum = (start + c + 0) & 0xFF;
    serial_->write(start);
    serial_->write(c);
    serial_->write((uint8_t)0x00);
    serial_->write(checksum);
  }

  // Send and try to read a response frame (non-blocking-friendly, but uses a small timeout)
  Response sendAndRead(Command cmd) {
    sendSimple(cmd);
    return readFrame();
  }

  // Read any incoming frame (AA, CMD, LEN, DATA, SM). Returns {valid=false} if timeout or invalid.
  Response readFrame() {
    Response r; r.valid = false;
    if (!serial_) return r;

    uint32_t t0 = millis();
    int stage = 0; // 0=wait AA, 1=CMD, 2=LEN, 3=DATA, 4=CHECK
    uint8_t cmd = 0, len = 0;
    std::vector<uint8_t> data;
    uint8_t sum = 0;

    while (millis() - t0 < timeoutMs_) {
      if (serial_->available()) {
        uint8_t b = serial_->read();
        switch (stage) {
          case 0: // wait for start byte
            if (b == 0xAA) { sum = b; stage = 1; }
            break;
          case 1: // read command
            cmd = b; sum += b; stage = 2; break;
          case 2: // read length
            len = b; sum += b; data.clear(); data.reserve(len); stage = (len == 0 ? 4 : 3); break;
          case 3: // read data bytes
            data.push_back(b); sum += b;
            if (data.size() == len) stage = 4; break;
          case 4: { // read checksum
            uint8_t sm = b; sum &= 0xFF;
            if (sm == sum) { r.cmd = cmd; r.data = std::move(data); r.valid = true; }
            return r; // finish regardless
          }
        }
      }
    }
    return r; // timeout
  }

private:
  HardwareSerial* serial_ = nullptr;
  uint32_t timeoutMs_ = 50;
  uint8_t defaultDrive_ = 0x01; // 0x01 = SD als standaard

};

// --- end of SV5W.h ---


