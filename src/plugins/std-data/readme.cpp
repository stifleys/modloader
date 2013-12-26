/*
 * Copyright (C) 2013  LINK/2012 <dma_2012@hotmail.com>
 * Licensed under GNU GPL v3, see LICENSE at top level directory.
 * 
 *  Readme reader -- the magic is simple
 *
 */
#include "data.h"

/*
 *  ReadmeReader 
 *      This object reads a readme file, yeah, that's right, a text file.
 *      Well, not like that, it actually reads each line trying to match the formating of that line with the formating of some data traits.
 * 
 *      Please, create the additional container for each trait only if necessary
 *          (when there's at least one readme with data for the specific trait)
 *      that will allow optimizations.
 * 
 */
struct ReadmeReader
{
    const char* filename;
    char buf[2048], *line;
    size_t line_number;
  
    /*
     *  Helper function to get the readme trait (the Additional trait in other words) from a fs object 
     */
    template<class T>
    auto get_trait(T& fs, const char* file_override) -> decltype(fs.Additional(0))
    {
        return fs.Additional(file_override);
    }
    
    
    /*
     *  try_to_handle_line
     *      It's here that the magic happens my friend 
     */
    
    template<size_t N = 0, class... Tp>
    typename std::enable_if<N == sizeof...(Tp)>::type    // Use this version if reached the tuple ending
    try_to_handle_line(std::tuple<Tp...>&)
    {
    }

    template<size_t N = 0, class... Tp>
    typename std::enable_if<(N < sizeof...(Tp))>::type   // Use this version if still didn't reach the tuple end
    try_to_handle_line(std::tuple<Tp...>& t)
    {
        // Useful typedefs
        typedef typename std::tuple<Tp...>                          tuple_type;
        typedef typename std::tuple_element<N, tuple_type>::type    pair_type;
        typedef typename std::decay<typename pair_type::first_type>::type pair_first_type;
        typedef pair_first_type                                     fs_type;
        typedef typename fs_type::traits_type                       trait_type;
        typedef typename trait_type::handler_type                   handler_type;
        
        // Get trait at position N from the tuple
        typename trait_type::container_type map;
        auto& xpair = std::get<N>(t);
        fs_type& fs = xpair.first;
        const char* overrideFile = xpair.second;
       
        
        // Try to add this line into the map from the trait...
        if(typename handler_type::Set()(ReadmeSection(), line, map) == false)
        {
            // Failed, the formating isn't compatible, try with the next trait in the tuple
            return try_to_handle_line<N + 1, Tp...>(t);
        }
        else
        {
            auto& trait = get_trait(fs, overrideFile);
            
            // Find out if we need to load the entire data from the original file before adding new things
            {
                int domflags = GetDomFlags(trait_type::domflags());

                /*  If the dominance flags say that if key do not exist in custom file it will be removed from the output,
                 *  we really need to load the original data into the trait */
                if((domflags & flag_RemoveIfNotExistInAnyCustom)
                || (domflags & flag_RemoveIfNotExistInOneCustomButInDefault))
                {
                    // We need to load the original data
                    if(!trait.isReady) trait.LoadData();
                }
                else
                {
                    // No need to load any data, the trait is ready!
                    trait.isReady = true;
                }
            }
            
            // Move contents from the map on this scope to the trait map
            std::move(map.begin(), map.end(), std::inserter(trait.map, trait.map.end()));
            
            // Successful! Log it.
            dataPlugin->Log("\tFound %s line at %d: %s", trait_type::what(), line_number, line);
            return;
        }
    }

    /*
     *  This object is a functor!
     *  Call me to read a readme file @filename
     */
    void operator()(const char* filename)
    {
        /*
         * XXX MAYBE (just maybe) it would be faster to read the entire file and parse from it... 
         * ...needs benchmark
         */
        
        dataPlugin->Log("Reading readme file %s", filename);
        
        // Setup my fields
        this->filename = filename;
        this->line_number = 0;
        
        // Setup the traits tuple
        auto  all_traits = std::make_tuple(
                                std::make_pair(std::ref(traits.gta), "data/gta.dat"),
                                std::make_pair(std::ref(traits.ide), "data/vehicles.ide")
                              );
        
        // Finally, open the file for reading...
        if(FILE* f = fopen(filename, "r"))
        {
            // ...read...
            for(; line = ParseConfigLine(fgets(buf, sizeof(buf), f)); ++line_number)
            {
                if(*line == 0) continue;
                try_to_handle_line(all_traits);
            }
            fclose(f);
        }
        
        // The magic is done ;)
    }
    
};


/*
 *  Reads the readme file @filename 
 */
void CThePlugin::ProcessReadmeFile(const char* filename)
{
    //return;
    ReadmeReader reader;
    return reader(filename);
}