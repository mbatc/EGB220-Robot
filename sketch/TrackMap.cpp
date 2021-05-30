#include "TrackMap.h"

void TrackMap::addSection(int avgMotorDiff, uint32_t length) {
  Section sec;
  sec.time = length;
  if (avgMotorDiff < -10)
    sec.type = ST_RTurn;
  else if (avgMotorDiff > 10)
    sec.type = ST_LTurn;
  else
    sec.type = ST_Straight;
  m_sections.append(sec);
}

uint32_t TrackMap::sectionLength(size_t index) {
  return m_sections[index].time;
}

TrackMap::SectionType TrackMap::sectionType(size_t index) {
  return m_sections[index].type;
}

// Get the number of sections
size_t TrackMap::sectionCount() {
  return m_sections.size();
}
