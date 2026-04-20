#pragma once

struct tagDpvrBaseInfo;

class CDpvrPositionCalc
{
    struct tagDpvrBaseInfo *m_pBaseInfo;
    int m_baseCnt;
    double m_d0;
    double m_d1;
public:
    CDpvrPositionCalc();
    ~CDpvrPositionCalc();
    void CreateBaseInfo(int n);
    void Position2Angle(float x, float y, float z, int baseIndex, float &m0, float &m1, float &m2);
    void Angle2Position(float m0, float m1, float m2, int baseIndex, float& x, float & y, float &z);
    void SetBaseQ(int idx, float x, float y, float z, float w);
    void SetBaseP(int idx, float x, float y, float z);
    void SetBaseAlpha1(int idx, float alpha1);
    void SetBaseParams(int idx, float q0x, float q0y, float q0z, float q1x, float q1y, float q1z, float s0, float s1, float s2, float a0, float a1, float a2, float b0, float b1, float b2);
    void SetBaseC(int idx, float c0, float c1, float c2);
    void Base2NormalCoordConv(int idx, float x, float y, float z, float &x_out, float &y_out, float &z_out);
    void Normal2BaseCoordConv(int idx, float x, float y, float z, float &x_out, float &y_out, float &z_out);
};

