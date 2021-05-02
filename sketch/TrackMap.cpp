#include "TrackMap.h"

TrackMap g_trackMap;

void TrackMap::addSection(int avgMotorDiff, uint32_t length) {
  Section sec;
  sec.time = length;
  sec.type = abs(avgMotorDiff) > 50 ? ST_Turn : ST_Straight;

  if (sec.type == ST_Turn)
    sec.targetSpeed = (1.0 - avgMotorDiff / 255.0) / 2;

  m_sections.append(sec);
}

float TrackMap::sectionSpeed(size_t index) {
  return m_sections[index].targetSpeed;
}

uint32_t TrackMap::sectionLength(size_t index) {
  return m_sections[index].time;
}

TrackMap::SectionType TrackMap::sectionType(size_t index) {
  return m_sections[index].type;
}
