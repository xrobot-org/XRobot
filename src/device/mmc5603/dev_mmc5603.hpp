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

  MMC5603(Rotation &rot);

  bool Init();

  void StartRecv();

  void PraseData();

  static int CaliCMD(MMC5603 *mmc5603, int argc, char **argv);

  float temp_;

  float intensity_;

  Rotation &rot_;

  Component::Type::Vector3 magn_, raw_magn_;

  Message::Topic<Component::Type::Vector3> magn_tp_;

  System::Term::Command<MMC5603 *> cmd_;

  System::Database::Key<Calibration> cali_data_;

  System::Semaphore raw_;

  System::Thread thread_;
};
}  // namespace Device
