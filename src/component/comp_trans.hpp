#pragma once

#include <comp_type.hpp>
#include <component.hpp>

namespace Component {
class Trans {
 public:
  typedef struct {
    float pit;
    float rol;
    float yaw;
  } Angle;

  static void EulrPosTrans(Angle &eulr, Type::Vector3 &pos) {
    float r1[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

    float r2[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

    float r3[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

    float cy = cosf(eulr.yaw);
    float sy = sinf(eulr.yaw);
    float cp = cosf(-eulr.pit);
    float sp = sinf(-eulr.pit);
    float cr = cosf(eulr.rol);
    float sr = sinf(eulr.rol);

    r1[0][0] = cp;
    r1[0][2] = sp;
    r1[2][0] = -sp;
    r1[2][2] = cp;

    r2[0][0] = cy;
    r2[0][1] = -sy;
    r2[1][0] = sy;
    r2[1][1] = cy;

    r3[1][1] = cr;
    r3[1][2] = -sr;
    r3[2][1] = sr;
    r3[2][2] = cr;

    // 计算向量坐标
    float v[3] = {pos.y, pos.x, pos.z};
    float temp[3] = {0, 0, 0};

    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        temp[i] += r3[i][j] * v[j];
      }
    }

    for (int i = 0; i < 3; i++) {
      v[i] = temp[i];
      temp[i] = 0;
    }

    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        temp[i] += r2[i][j] * v[j];
      }
    }

    for (int i = 0; i < 3; i++) {
      v[i] = temp[i];
      temp[i] = 0;
    }

    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        temp[i] += r1[i][j] * v[j];
      }
    }

    // 返回结果
    pos.x = temp[1];
    pos.y = temp[0];
    pos.z = temp[2];
  }
};
}  // namespace Component
