#include "TrackMap.h"

void TrackMap::addSection(int avgMotorDiff, uint32_t length) {
  SectionType secType = 0;
  if (avgMotorDiff < -10)
    secType = ST_LTurn;
  else if (avgMotorDiff > 10)
    secType = ST_RTurn;
  else
    secType = ST_Straight;

  if (m_size > 0 && secType == m_sections[m_size - 1].type)
  {
    m_sections[m_size - 1].time += length;
  }
  else
  {
    m_sections[m_size].type = secType;
    m_sections[m_size].time = length;
    ++m_size;
  }
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
