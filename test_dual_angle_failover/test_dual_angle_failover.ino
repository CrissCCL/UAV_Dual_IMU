#include <Arduino.h>
#include <Wire.h>

// =====================
// IMU selection (auto failover)
// =====================
enum ImuSelect { IMU_PRIMARY_BMI, IMU_PRIMARY_ICM };
static ImuSelect PRIMARY_IMU = IMU_PRIMARY_BMI;   // arranque

// =====================
// Direcciones I2C
// =====================
static const uint8_t BMI_ACC_ADDR = 0x18;
static const uint8_t BMI_GYR_ADDR = 0x68;
static const uint8_t ICM_ADDR     = 0x69;

// I2C clocks
static const uint32_t I2C0_FREQ = 400000; // Wire  (BMI)
static const uint32_t I2C1_FREQ = 100000; // Wire1 (ICM robust)

// =====================
// Estado
// =====================
bool bmi_ok=false, icm_ok=false;
bool bmi_data_ok=false, icm_data_ok=false;

// Raw->units por IMU
float bmi_ax=0,bmi_ay=0,bmi_az=0, bmi_gx=0,bmi_gy=0,bmi_gz=0; // g, deg/s
float icm_ax=0,icm_ay=0,icm_az=0, icm_gx=0,icm_gy=0,icm_gz=0; // g, deg/s

// Primary outputs (para tu controlador después)
float AccX=0,AccY=0,AccZ=0;
float RateRoll=0,RatePitch=0,RateYaw=0;

// =====================
// Helpers
// =====================
static inline int16_t le16(uint8_t lsb, uint8_t msb) { return (int16_t)((msb << 8) | lsb); }
static inline int16_t be16(uint8_t msb, uint8_t lsb) { return (int16_t)((msb << 8) | lsb); }
static inline float   rad2deg(float r){ return r * 57.2957795f; }

// =====================
// BMI088 regs
// =====================
static const uint8_t BMI_ACC_DATA_X_L  = 0x12;
static const uint8_t BMI_ACC_RANGE     = 0x41;
static const uint8_t BMI_ACC_PWR_CONF  = 0x7C;
static const uint8_t BMI_ACC_PWR_CTRL  = 0x7D;
static const uint8_t BMI_ACC_SOFTRESET = 0x7E;

static const uint8_t BMI_GYR_RATE_X_L  = 0x02;
static const uint8_t BMI_GYR_RANGE     = 0x0F;
static const uint8_t BMI_GYR_BANDWIDTH = 0x10;
static const uint8_t BMI_GYR_LPM1      = 0x11;
static const uint8_t BMI_GYR_SOFTRESET = 0x14;

// =====================
// ICM-42605 regs (Bank0)
// =====================
static const uint8_t ICM_BANK_SEL       = 0x76;
static const uint8_t ICM_DEVICE_CONFIG  = 0x11;
static const uint8_t ICM_WHO_AM_I       = 0x75; // 0x42
static const uint8_t ICM_INTF_CONFIG1   = 0x4D;
static const uint8_t ICM_PWR_MGMT0      = 0x4E;
static const uint8_t ICM_GYRO_CONFIG0   = 0x4F;
static const uint8_t ICM_ACCEL_CONFIG0  = 0x50;
static const uint8_t ICM_ACCEL_DATA_X1  = 0x1F;
static const uint8_t ICM_GYRO_DATA_X1   = 0x25;

// =====================
// Wire (BMI) helpers
// =====================
static bool w_write8(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire.write(reg); Wire.write(val);
  return (Wire.endTransmission(true) == 0);
}
static bool w_readN(uint8_t addr, uint8_t startReg, uint8_t *buf, size_t n) {
  Wire.beginTransmission(addr);
  Wire.write(startReg);
  if (Wire.endTransmission(false) != 0) return false;
  size_t got = Wire.requestFrom((int)addr, (int)n);
  if (got != n) return false;
  for (size_t i=0;i<n;i++) buf[i]=Wire.read();
  return true;
}

// =====================
// Wire1 (ICM) robust helpers
// =====================
static bool w1_write8_stop(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire1.beginTransmission(addr);
  Wire1.write(reg); Wire1.write(val);
  return (Wire1.endTransmission(true) == 0);
}
static bool w1_read8_stop(uint8_t addr, uint8_t reg, uint8_t &val) {
  Wire1.beginTransmission(addr);
  Wire1.write(reg);
  if (Wire1.endTransmission(true) != 0) return false;
  delayMicroseconds(80);
  if (Wire1.requestFrom((int)addr, 1) != 1) return false;
  val = Wire1.read();
  return true;
}
static bool w1_read6_bytewise(uint8_t addr, uint8_t startReg, uint8_t out6[6]) {
  for (int i=0;i<6;i++) {
    if (!w1_read8_stop(addr, startReg + i, out6[i])) return false;
  }
  return true;
}

// =====================
// BMI init + read
// =====================
static bool bmi_init() {
  if (!w_write8(BMI_ACC_ADDR, BMI_ACC_SOFTRESET, 0xB6)) return false;
  delay(2);
  if (!w_write8(BMI_ACC_ADDR, BMI_ACC_PWR_CONF, 0x00)) return false;
  delay(1);
  if (!w_write8(BMI_ACC_ADDR, BMI_ACC_PWR_CTRL, 0x04)) return false;
  delay(5);
  if (!w_write8(BMI_ACC_ADDR, BMI_ACC_RANGE, 0x01)) return false; // +/-6g

  if (!w_write8(BMI_GYR_ADDR, BMI_GYR_SOFTRESET, 0xB6)) return false;
  delay(35);
  if (!w_write8(BMI_GYR_ADDR, BMI_GYR_LPM1, 0x00)) return false;
  delay(1);
  if (!w_write8(BMI_GYR_ADDR, BMI_GYR_RANGE, 0x00)) return false; // +/-2000 dps
  if (!w_write8(BMI_GYR_ADDR, BMI_GYR_BANDWIDTH, 0x02)) return false;

  return true;
}

static bool bmi_read_units(float &ax_g, float &ay_g, float &az_g,
                           float &gx_dps, float &gy_dps, float &gz_dps) {
  uint8_t a[6], g[6];
  if (!w_readN(BMI_ACC_ADDR, BMI_ACC_DATA_X_L, a, 6)) return false;
  if (!w_readN(BMI_GYR_ADDR, BMI_GYR_RATE_X_L, g, 6)) return false;

  int16_t ax = le16(a[0],a[1]);
  int16_t ay = le16(a[2],a[3]);
  int16_t az = le16(a[4],a[5]);

  int16_t gx = le16(g[0],g[1]);
  int16_t gy = le16(g[2],g[3]);
  int16_t gz = le16(g[4],g[5]);

  const float ACC_LSB_PER_G   = 5460.0f;
  const float GYR_LSB_PER_DPS = 16.4f;

  ax_g = (float)ax / ACC_LSB_PER_G;
  ay_g = (float)ay / ACC_LSB_PER_G;
  az_g = (float)az / ACC_LSB_PER_G;

  gx_dps = (float)gx / GYR_LSB_PER_DPS;
  gy_dps = (float)gy / GYR_LSB_PER_DPS;
  gz_dps = (float)gz / GYR_LSB_PER_DPS;

  return true;
}

// =====================
// ICM init + read
// =====================
static bool icm_init_strict() {
  if (!w1_write8_stop(ICM_ADDR, ICM_BANK_SEL, 0x00)) return false;
  delay(1);

  if (!w1_write8_stop(ICM_ADDR, ICM_DEVICE_CONFIG, 0x01)) return false; // reset
  delay(3);

  if (!w1_write8_stop(ICM_ADDR, ICM_BANK_SEL, 0x00)) return false;
  delay(1);

  uint8_t who=0;
  if (!w1_read8_stop(ICM_ADDR, ICM_WHO_AM_I, who)) return false;
  if (who != 0x42) return false;

  (void)w1_write8_stop(ICM_ADDR, ICM_INTF_CONFIG1, 0x01); // disable I3C
  delay(1);

  if (!w1_write8_stop(ICM_ADDR, ICM_GYRO_CONFIG0,  0x06)) return false;
  if (!w1_write8_stop(ICM_ADDR, ICM_ACCEL_CONFIG0, 0x06)) return false;

  if (!w1_write8_stop(ICM_ADDR, ICM_PWR_MGMT0, 0x0F)) return false; // accel+gyro LN
  delay(70);

  return true;
}

static bool icm_read_units(float &ax_g, float &ay_g, float &az_g,
                           float &gx_dps, float &gy_dps, float &gz_dps) {
  (void)w1_write8_stop(ICM_ADDR, ICM_BANK_SEL, 0x00);
  delayMicroseconds(80);

  uint8_t pm=0;
  if (w1_read8_stop(ICM_ADDR, ICM_PWR_MGMT0, pm)) {
    if (pm != 0x0F) {
      (void)w1_write8_stop(ICM_ADDR, ICM_PWR_MGMT0, 0x0F);
      delay(70);
    }
  }

  uint8_t a[6], g[6];
  if (!w1_read6_bytewise(ICM_ADDR, ICM_ACCEL_DATA_X1, a)) return false;
  if (!w1_read6_bytewise(ICM_ADDR, ICM_GYRO_DATA_X1,  g)) return false;

  int16_t ax = be16(a[0],a[1]);
  int16_t ay = be16(a[2],a[3]);
  int16_t az = be16(a[4],a[5]);

  int16_t gx = be16(g[0],g[1]);
  int16_t gy = be16(g[2],g[3]);
  int16_t gz = be16(g[4],g[5]);

  const float ACC_LSB_PER_G   = 2048.0f;
  const float GYR_LSB_PER_DPS = 16.4f;

  ax_g = (float)ax / ACC_LSB_PER_G;
  ay_g = (float)ay / ACC_LSB_PER_G;
  az_g = (float)az / ACC_LSB_PER_G;

  gx_dps = (float)gx / GYR_LSB_PER_DPS;
  gy_dps = (float)gy / GYR_LSB_PER_DPS;
  gz_dps = (float)gz / GYR_LSB_PER_DPS;

  return true;
}

// =====================
// Init + update
// =====================
static void imu_init() {
  Wire.begin();
  Wire.setClock(I2C0_FREQ);

  Wire1.setSDA(17);
  Wire1.setSCL(16);
  Wire1.begin();
  Wire1.setClock(I2C1_FREQ);

  bmi_ok = bmi_init();
  icm_ok = icm_init_strict();
}

static void set_primary(ImuSelect sel) {
  PRIMARY_IMU = sel;
}

static void imu_update() {
  bmi_data_ok = bmi_read_units(bmi_ax,bmi_ay,bmi_az, bmi_gx,bmi_gy,bmi_gz);
  icm_data_ok = icm_read_units(icm_ax,icm_ay,icm_az, icm_gx,icm_gy,icm_gz);

  if (PRIMARY_IMU == IMU_PRIMARY_BMI) {
    AccX=bmi_ax; AccY=bmi_ay; AccZ=bmi_az;
    RateRoll=bmi_gx; RatePitch=bmi_gy; RateYaw=bmi_gz;
  } else {
    AccX=icm_ax; AccY=icm_ay; AccZ=icm_az;
    RateRoll=icm_gx; RatePitch=icm_gy; RateYaw=icm_gz;
  }
}

// =====================
// Angles (complementary per IMU)
// =====================
static float bmi_roll=0, bmi_pitch=0;
static float icm_roll=0, icm_pitch=0;
static uint32_t t_prev=0;

// =====================
// Mismatch + failover config
// =====================
static const float MISMATCH_DEG   = 5.0f;   // umbral grados
static const int   MISMATCH_HITS  = 10;     // ciclos seguidos
static const uint32_t FAILOVER_COOLDOWN_MS = 1500; // no cambiar de nuevo por X ms

static int mismatch_counter = 0;
static bool imu_mismatch = false;
static uint32_t last_failover_ms = 0;

static void mismatch_update(float dRoll, float dPitch) {
  // mismatch logic: solo si ambas están OK
  if (bmi_data_ok && icm_data_ok) {
    if (fabsf(dRoll) > MISMATCH_DEG || fabsf(dPitch) > MISMATCH_DEG) mismatch_counter++;
    else mismatch_counter--;
  } else {
    mismatch_counter++; // si una cae, cuenta mismatch
  }

  if (mismatch_counter < 0) mismatch_counter = 0;
  if (mismatch_counter > MISMATCH_HITS) mismatch_counter = MISMATCH_HITS;

  imu_mismatch = (mismatch_counter >= MISMATCH_HITS);
}

static void maybe_failover() {
  if (!imu_mismatch) return;

  uint32_t now = millis();
  if (now - last_failover_ms < FAILOVER_COOLDOWN_MS) return;

  // Si la primaria es BMI y ICM está OK -> cambia a ICM
  if (PRIMARY_IMU == IMU_PRIMARY_BMI) {
    if (icm_data_ok) {
      set_primary(IMU_PRIMARY_ICM);
      last_failover_ms = now;
      mismatch_counter = 0; // resetea para evitar rebote inmediato
      Serial.println(">>> FAILOVER: PRIMARY -> ICM42605");
    }
  } else {
    // primaria es ICM, si BMI está OK -> cambia a BMI
    if (bmi_data_ok) {
      set_primary(IMU_PRIMARY_BMI);
      last_failover_ms = now;
      mismatch_counter = 0;
      Serial.println(">>> FAILOVER: PRIMARY -> BMI088");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  imu_init();

  Serial.println("Teensy 4.0 - Dual IMU compare + mismatch + auto failover");
  Serial.print("START PRIMARY_IMU = "); Serial.println(PRIMARY_IMU == IMU_PRIMARY_BMI ? "BMI088" : "ICM42605");
  Serial.print("BMI init: "); Serial.println(bmi_ok ? "OK" : "FAIL");
  Serial.print("ICM init: "); Serial.println(icm_ok ? "OK" : "FAIL");

  t_prev = micros();
}

void loop() {
  // dt real
  uint32_t t_now = micros();
  float dt = (t_now - t_prev) * 1e-6f;
  if (dt <= 0) dt = 0.005f;
  if (dt > 0.05f) dt = 0.005f;
  t_prev = t_now;

  imu_update();

  // Angulos ACC
  float bmi_roll_acc  = rad2deg(atan2f(bmi_ay, sqrtf(bmi_ax*bmi_ax + bmi_az*bmi_az)));
  float bmi_pitch_acc = -rad2deg(atan2f(bmi_ax, sqrtf(bmi_ay*bmi_ay + bmi_az*bmi_az)));

  float icm_roll_acc  = rad2deg(atan2f(icm_ay, sqrtf(icm_ax*icm_ax + icm_az*icm_az)));
  float icm_pitch_acc = -rad2deg(atan2f(icm_ax, sqrtf(icm_ay*icm_ay + icm_az*icm_az)));

  // Complementary
  const float alpha = 0.98f;

  if (bmi_data_ok) {
    bmi_roll  = alpha*(bmi_roll  + bmi_gx*dt) + (1.0f-alpha)*bmi_roll_acc;
    bmi_pitch = alpha*(bmi_pitch + bmi_gy*dt) + (1.0f-alpha)*bmi_pitch_acc;
  }
  if (icm_data_ok) {
    icm_roll  = alpha*(icm_roll  + icm_gx*dt) + (1.0f-alpha)*icm_roll_acc;
    icm_pitch = alpha*(icm_pitch + icm_gy*dt) + (1.0f-alpha)*icm_pitch_acc;
  }

  // Delta + mismatch + failover
  float dRoll  = bmi_roll  - icm_roll;
  float dPitch = bmi_pitch - icm_pitch;

  mismatch_update(dRoll, dPitch);
  maybe_failover();

  // Print ~100 Hz
  static uint32_t t_print=0;
  if (millis() - t_print >= 10) {
    t_print = millis();

    Serial.print("P=");
    Serial.print(PRIMARY_IMU == IMU_PRIMARY_BMI ? "BMI" : "ICM");
    Serial.print("  ");

    Serial.print("BMI("); Serial.print(bmi_data_ok); Serial.print(") ");
    Serial.print("r,p="); Serial.print(bmi_roll,1); Serial.print(",");
    Serial.print(bmi_pitch,1);

    Serial.print("  ||  ");

    Serial.print("ICM("); Serial.print(icm_data_ok); Serial.print(") ");
    Serial.print("r,p="); Serial.print(icm_roll,1); Serial.print(",");
    Serial.print(icm_pitch,1);

    Serial.print("  ||  ");
    Serial.print("d="); Serial.print(dRoll,1); Serial.print(",");
    Serial.print(dPitch,1);

    Serial.print("  ||  ");
    Serial.print("MISMATCH="); Serial.print(imu_mismatch ? "YES" : "NO");
    Serial.print(" (cnt="); Serial.print(mismatch_counter); Serial.print(")");

    Serial.println();
  }
}
