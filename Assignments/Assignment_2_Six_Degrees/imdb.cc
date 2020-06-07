#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h";
#include <cstring>

using namespace std;

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

imdb::imdb(const string& directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;

  cout << actorFileName << endl;
  cout << movieFileName << endl;
  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

// key for binary search function in getCredits
// player is the name of the actor to search for.
// base address is the base address of the actor file
struct actorKey {
    char const* player;
    const void* baseAddress;
};

/**
   * Helper function: comparePlayers
   * ------------------
   * Helper function to getCredits(), passed as argument to bsearch in c std lib.
   * Compares the names of two players stored as c-strings.
   *
   * @param key as actorKey* containing the name of the actor and the base address of the actorFile.
   * @param element the offset of an address of an actor name to the base address actorFile.
   * @return 0 if and only if the player's names (c-strings) are bitwise equal,
   *        and -1 otherwise.
   */
int comparePlayers(const void* key, const void* element) {
    actorKey actorInfo = *(actorKey*)key;
    char const* keyName = actorInfo.player;
    const char* elementName = (char*)actorInfo.baseAddress + *(int*)element;
    return strcmp(keyName, elementName);
}

// you should be implementing these two methods right here... 
bool imdb::getCredits(const string& player, vector<film>& films) const { 
    void* actorOffset; // address of the actor found in actorFile or null if not found
    char const* playerC = player.c_str(); 
    actorKey key;
    key.player = playerC;
    key.baseAddress = actorFile;

    // find address of player in the actor file
    // bsearch(key, base address of array of int offsets to actor names, size of int offsets, 
    //      number of actors in the record, comparison function between actor offsets)
    actorOffset = bsearch(&key, (char*)actorFile + 4, *(int*)actorFile, 4, comparePlayers);

    // player not found in the actor file
    if (!actorOffset) {
        return false;
    }

    // get list of movies that player played in
    char* actorRecord = (char*)actorFile + *(int*)actorOffset; 

    // go to number of movies the actor played in in actor record
    // if amount of characters of actor's name is even,
    // an extra zero is added for padding as well as the extra
    // null character for a c-string
    int playerLength;
    if ((player.size() % 2) == 0) { 
        playerLength = player.size() + 2;      
    }
    else {
        playerLength = player.size() + 1;
    }
    actorRecord += playerLength;
    short moviesLength = *(short*)actorRecord;   
    films.resize(moviesLength);

    // go to first film offset in actor record
    // if the number of bytes dedicated to the player's name +
    // the 2 bytes storing the number of movies is not a multiple of four
    // then 2 bytes are added as padding after the number of movies
    if ((playerLength + 2) % 4) {
        actorRecord += 4;
    }
    else {
        actorRecord += 2;
    }

    for (size_t i = 0; i < moviesLength; i++) {
        film actorFilm;
        char* filmRecord = (char*)movieFile + *(int*)actorRecord; // filmRecord start at address movieFile + offset
        actorFilm.title = filmRecord; 
        filmRecord += actorFilm.title.size() + 1; // go to film year filmsize + 1 bytes further (extra null character)
        actorFilm.year = 1900 + *filmRecord;
        films[i] = actorFilm;
        actorRecord += 4; // go 4 bytes ahead to next film offset
    }

    return true;
}

// key for binary search function in getCast
// movie is the name of the movie to search for.
// base address is the base address of the movie file
struct movieKey {
    film movie;
    const void* baseAddress;
};

/**
   * Helper function: compareMovies
   * ------------------
   * Helper function to getCast(), passed as argument to bsearch in c std lib.
   * Compares two movies as stored as films.
   *
   * @param key as movieKey* containing the movie as film and the base address of the movieFile.
   * @param element the offset of an address of a movie as film to the base address movieFile.
   * @return 0 if the movies are equal, -1 if the element is smaller than the key
   *        and 1 otherwise.
   */
int compareMovies(const void* key, const void* element) {
    int result;
    film movie;
    film keyFilm = ((movieKey*)key)->movie;

    const char* movieEntry = (char*)((movieKey*)key)->baseAddress + *(int*)element;
    movie.title = movieEntry;
    
    // move to year of movie in movie file
    while (*movieEntry != '\0') {
        movieEntry += 1; 
    }
    movieEntry += 1;
    movie.year = *movieEntry + 1900; // number in movie file is an offset to year 1900

    if (keyFilm < movie) {
        result = -1;
    }
    else if (keyFilm == movie) {
        result = 0;
    }
    else {
        result = 1;
    }

    return result;
}

bool imdb::getCast(const film& movie, vector<string>& players) const {
    void* movieOffset; // address of the movie found in movieFile or null if not found
    movieKey key;
    key.movie = movie;
    key.baseAddress = movieFile;

    // find address of movie in the movie file
    // bsearch(key, base address of array of int offsets to movie names, size of int offsets, 
    //      number of movies in the record, comparison function between movies offsets)
    movieOffset = bsearch(&key, (char*)movieFile + 4, *(int*)movieFile, 4, compareMovies);

    // movie not found in the movie file
    if (!movieOffset) {
        players.clear();
        return false;
    }

    char* movieRecord = (char*)movieFile + *(int*)movieOffset;

    // go to number of players in the movie's cast
    // if amount of characters of movie's title + null character + byte for the year is odd,
    // an extra zero is added for padding
    int movieLength;
    if ((movie.title.size() % 2) == 0) {
        movieLength = movie.title.size() + 2;
    }
    else {
        movieLength = movie.title.size() + 3;
    }
    movieRecord += movieLength;

    short castSize = *(short*)movieRecord;
    players.resize(castSize);

    // go to first actor offset in movie record
    // if the number of bytes dedicated to the movie's title + year + 
    // the 2 bytes storing the number of players is not a multiple of four
    // then 2 bytes are added as padding after the number of players
    if ((movieLength + 2) % 4) {
        movieRecord += 4;
    }
    else {
        movieRecord += 2;
    }

    for (size_t i = 0; i < castSize; i++) {
        string actor = (char*)actorFile + *(int*)movieRecord; // actor name starts at address actorFile + offset
        players[i] = actor;
        movieRecord += 4; // go 4 bytes ahead to next actor offset
    }

    return true;
}

imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}
