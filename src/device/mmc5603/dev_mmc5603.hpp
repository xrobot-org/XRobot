#include "device.hpp"

namespace Device {
class MMC5603 {
 public:
  typedef struct {
    /* 旋转矩阵 */
    float rot_mat[3][3];
  } Rotation;

  typedef struct {
    Component::Type::Vector3 offset; /* 偏置 */
    Component::Type::Vector3 scale;  /* 偏置 */
  } Calibration;

  typedef enum {
    DATA_RATE_75,
    DATA_RATE_150,
    DATA_RATE_255,
  } DataRate;

  MMC5603(Rotation &rot, float max_offset_ = 0.1f,
          DataRate data_rate = DATA_RATE_255);

  bool Init(DataRate date_rate);

  void StartRecv();

  void PraseData();

  static int CaliCMD(MMC5603 *mmc5603, int argc, char **argv);

  float temp_;

  float intensity_;

  Rotation &rot_;

  DataRate data_rate_;

  float max_offset_;

  Component::Type::Vector3 magn_, raw_magn_;

  Message::Topic<Component::Type::Vector3> magn_tp_;
  Message::Topic<Component::Type::Vector3> raw_magn_tp_;

  System::Term::Command<MMC5603 *> cmd_;

  System::Database::Key<Calibration> cali_data_;

  System::Semaphore raw_;

  System::Thread thread_;
};
}  // namespace Device
