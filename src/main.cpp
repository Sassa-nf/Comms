#include <Arduino.h>
#include <Wire.h>
#include <comms.h>
#include <config.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

Config cfg;

ScanTask report_scans = ScanTask{
  count : 0,
  delay : 0
};

struct scan_result
{
  uint64_t last_epoch;

  ScanTaskResult dump;
  size_t readings_sz;
} scan_result;

TaskHandle_t logToSerial = NULL;

void dumpToCommsTask(void *);
void checkComms(void);
void pushEvents(int);
void dumpToComms(void);

void setup()
{
  Serial.begin(115200);

  cfg = Config::init();
  Comms::initComms(cfg);
  scan_result.last_epoch = 0;
  scan_result.readings_sz = 0;

  xTaskCreate(dumpToCommsTask, "DUMP_RESPONSE_PROCESS", 2048, NULL, 1, &logToSerial);

  Serial.println("setup complete");
}

void loop()
{
  checkComms();
  delay(500);

  Serial.println("Epoch: " + String(scan_result.last_epoch));
  pushEvents(256);
}

void checkComms()
{
  while (Comms0->available() > 0)
  {
    Message *m = Comms0->receive();
    if (m == NULL)
      continue;

    switch (m->type)
    {
    case MessageType::SCAN:
      report_scans = m->payload.scan;
      break;
    }
    delete m;
  }
}

void pushEvents(int events)
{
  scan_result.last_epoch++;
  scan_result.dump.sz = events;

  if (scan_result.dump.sz > scan_result.readings_sz)
  {
    size_t old_sz = scan_result.readings_sz;
    scan_result.readings_sz = scan_result.dump.sz;
    uint32_t *f = new uint32_t[scan_result.readings_sz];
    int16_t *r = new int16_t[scan_result.readings_sz];

    if (old_sz > 0)
    {
      memcpy(f, scan_result.dump.v0,
             old_sz * sizeof(uint32_t));
      memcpy(r, scan_result.dump.v1, old_sz * sizeof(int16_t));

      delete[] scan_result.dump.v0;
      delete[] scan_result.dump.v1;
    }

    scan_result.dump.v0 = f;
    scan_result.dump.v1 = r;
  }

  for (int i = 0; i < events; i++)
  {
    scan_result.dump.v0[i] = 2000000 + i;
    scan_result.dump.v1[i] = -10 - i;
  }

  xTaskNotifyGive(logToSerial);
}

void dumpToCommsTask(void *parameter)
{
  uint64_t last_epoch = scan_result.last_epoch;

  for (;;)
  {
    int64_t delay = report_scans.delay;
    if (delay == 0)
    {
      delay = (1 << 63) - 1;
    }

    ulTaskNotifyTake(true, pdMS_TO_TICKS(delay));
    if (report_scans.count == 0 || scan_result.last_epoch == last_epoch)
    {
      continue;
    }

    if (report_scans.count > 0)
    {
      report_scans.count--;
    }

    dumpToComms();
  }
}

void dumpToComms()
{
  Message m;
  m.type = MessageType::SCAN_RESULT;
  m.payload.dump = scan_result.dump;
  Comms0->send(m);
}
