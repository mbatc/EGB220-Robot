#include "TrackMap.h"

void TrackMap::addSection(int avgMotorDiff, uint32_t length) {
  Section &sec = m_sections[m_size];
  sec.time = length;
  if (avgMotorDiff < -10)
    sec.type = ST_RTurn;
  else if (avgMotorDiff > 10)
    sec.type = ST_LTurn;
  else
    sec.type = ST_Straight;
  ++m_size;
}

uint32_t TrackMap::sectionLength(size_t index) {
  return m_sections[index].time;
}

TrackMap::SectionType TrackMap::sectionType(size_t index) {
  return m_sections[index].type;
}

size_t TrackMap::sectionCount() {
  return m_size;
}

void TrackMap::clear() {
  m_size = 0;
}
