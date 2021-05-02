#ifndef TrackMap_h__
#define TrackMap_h__

#include "List.h"

class TrackMap
{
public:
  enum SectionType
  {
    ST_Straight,
    ST_Turn
  };

  struct Section
  {
    SectionType type; // Type of section
    float targetSpeed; // Target speed for the section
    uint32_t time;  // Time taken to complete the section
  };

  // Add a new section of track to the map.
  // The type of section is determined from the average difference
  // in motor speed for the section.
  void addSection(int avgMotorDiff, uint32_t length);

  // Get the target speed for a section of the track
  float sectionSpeed(size_t index);

  // Get the time taken to complete the section of track
  uint32_t sectionLength(size_t index);

  // Get the type of a section of track
  SectionType sectionType(size_t index);

  // Get the number of sections
  size_t sectionCount();

private:
  List<Section> m_sections;
};

extern TrackMap g_trackMap;

#endif // TrackMap_h__