#ifndef TrackMap_h__
#define TrackMap_h__

#include "List.h"

#define MAX_SECTIONS 32

class TrackMap
{
public:
  enum SectionType
  {
    ST_Straight = 0,
    ST_LTurn    = 1,
    ST_RTurn    = 2
  };

  struct Section
  {
    SectionType type; // Type of section
    uint32_t time;  // Time taken to complete the section
  };

  // Add a new section of track to the map.
  // The type of section is determined from the average difference
  // in motor speed for the section.
  void addSection(int avgMotorDiff, uint32_t length);

  // Get the time taken to complete the section of track
  uint32_t sectionLength(size_t index);

  // Get the type of a section of track
  SectionType sectionType(size_t index);

  // Get the number of sections
  size_t sectionCount();

  // Clear the track map
  void clear();

private:
  size_t  m_size = 0;
  Section m_sections[MAX_SECTIONS];
};

#endif // TrackMap_h__
