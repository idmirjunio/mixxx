#pragma once

#include "track/trackid.h"
#include "util/compatibility/qhash.h"
#include "util/fileinfo.h"

// A track in the library is identified by a location and an id.
// The location is mandatory to identify the file, whereas the id
// only exists after the track has been inserted into the database.
//
// This class is intended to be used as a simple, almost immutable
// value object. Only the id can be set once.
class TrackRef final {
  public:
    /// Converts a file path and an optional TrackId into a TrackRef.
    ///
    /// This involves and intermediate creation of mixxx::FileInfo
    /// and accessing the file system!
    static TrackRef fromFilePath(
            const QString& filePath,
            TrackId id = TrackId()) {
        return fromFileInfo(mixxx::FileInfo(filePath), id);
    }

    /// Converts a mixxx::FileInfo and an optional TrackId into a TrackRef.
    ///
    /// This involves obtaining the file-related track properties from
    /// the file info and might involve accessing the file system!
    static TrackRef fromFileInfo(
            mixxx::FileInfo fileInfo,
            TrackId id = TrackId()) {
        // The conditional refresh ensures that files that were previously
        // unavailable (e.g. file system volume not mounted before) are
        // resolved successfully.
        auto canonicalLocation = fileInfo.resolveCanonicalLocation();
        // All properties of the file info are now considered fresh
        return TrackRef(
                fileInfo.location(),
                std::move(canonicalLocation),
                std::move(id));
    }

    // Default constructor
    TrackRef() {
        DEBUG_ASSERT(verifyConsistency());
    }
    // Regular copy constructor
    TrackRef(const TrackRef& other) = default;
    // Custom copy constructor:  Creates a copy of an existing TrackRef,
    // but overwrite the TrackId with a custom value.
    TrackRef(
            const TrackRef& other,
            TrackId id)
        : m_location(other.m_location),
          m_canonicalLocation(other.m_canonicalLocation),
          m_id(id) {
        DEBUG_ASSERT(verifyConsistency());
    }

    // The human-readable identifier of a track in Mixxx. The location is
    // immutable and the starting point for accessing a track's file.
    const QString& getLocation() const {
        return m_location;
    }
    bool hasLocation() const {
        return !getLocation().isEmpty();
    }

    // The unique identifier of a track's file at runtime and used
    // for caching. The canonical location is empty for inexistent
    // files.
    const QString& getCanonicalLocation() const {
        return m_canonicalLocation;
    }
    bool hasCanonicalLocation() const {
        return !getCanonicalLocation().isEmpty();
    }

    // The primary key of a track in the Mixxx library. The id must only
    // be set once after inserting into or after loading from the database.
    // Tracks that have not been stored in the database don't have an id.
    const TrackId& getId() const {
        return m_id;
    }
    bool hasId() const {
        return getId().isValid();
    }

    bool isValid() const {
        return hasId() || hasCanonicalLocation();
    }

protected:
    // Initializing constructor
    TrackRef(
            const QString& location,
            const QString& canonicalLocation,
            TrackId id = TrackId())
        : m_location(location),
          m_canonicalLocation(canonicalLocation),
          m_id(id) {
        DEBUG_ASSERT(verifyConsistency());
    }

private:
    // Checks if all class invariants are met
    bool verifyConsistency() const;

    QString m_location;
    QString m_canonicalLocation;
    TrackId m_id;
};

inline
bool operator==(const TrackRef& lhs, const TrackRef& rhs) {
    return (lhs.getId() == rhs.getId()) &&
            (lhs.getLocation() == rhs.getLocation()) &&
            (lhs.getCanonicalLocation() == rhs.getCanonicalLocation());
}

inline
bool operator!=(const TrackRef& lhs, const TrackRef& rhs) {
    return !(lhs == rhs);
}

Q_DECLARE_METATYPE(TrackRef)

std::ostream& operator<<(std::ostream& os, const TrackRef& trackRef);

QDebug operator<<(QDebug debug, const TrackRef& trackRef);

inline qhash_seed_t qHash(
        const TrackRef& key,
        qhash_seed_t seed = 0) {
    return qHash(
            key.getLocation(), seed) ^
            qHash(key.getId(), seed);
}
