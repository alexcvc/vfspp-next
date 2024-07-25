#ifndef VIRTUALFILESYSTEM_H
#define VIRTUALFILESYSTEM_H

#include "IFileSystem.h"
#include "IFile.h"

namespace vfspp
{

using VirtualFileSystemPtr = std::shared_ptr<class VirtualFileSystem>;
using VirtualFileSystemWeakPtr = std::weak_ptr<class VirtualFileSystem>;
    

class VirtualFileSystem final
{
public:
    typedef std::list<IFileSystemPtr> TFileSystemList;
    typedef std::unordered_map<std::string, IFileSystemPtr> TFileSystemMap;
    
public:
    VirtualFileSystem()
    {
    }

    ~VirtualFileSystem()
    {
        std::for_each(m_FileSystem.begin(), m_FileSystem.end(), [](const TFileSystemMap::value_type& fs) {
            fs.second->Shutdown();
        });
    }
    
    /*
     * Register new filesystem. Alias is a base prefix to file access.
     * For ex. registered filesystem has base path '/home/media', but registered
     * with alias '/', so it possible to access files with path '/filename'
     * instead of '/home/media/filename
     */
    void AddFileSystem(std::string alias, IFileSystemPtr filesystem)
    {
        if (!filesystem) {
            return;
        }

        if (!StringUtils::EndsWith(alias, "/")) {
            alias += "/";
        }
        
        m_FileSystem[alias] = filesystem;
        m_SortedAlias.push_back(SSortedAlias(alias, filesystem));
        m_SortedAlias.sort([](const SSortedAlias& a1, const SSortedAlias& a2) {
            return a1.alias.length() > a2.alias.length();
        });
    }
    
    /*
     * Remove registered filesystem
     */
    void RemoveFileSystem(std::string alias)
    {
        if (!StringUtils::EndsWith(alias, "/")) {
            alias += "/";
        }

        m_FileSystem.erase(alias);
        m_SortedAlias.remove_if([alias](const SSortedAlias& a) {
            return a.alias == alias;
        });
    }
    
    /*
     * Check if filesystem with 'alias' registered
     */
    bool IsFileSystemExists(std::string alias) const
    {
        if (!StringUtils::EndsWith(alias, "/")) {
            alias += "/";
        }

        return (m_FileSystem.find(alias) != m_FileSystem.end());
    }
    
    /*
     * Get filesystem with 'alias'
     */
    IFileSystemPtr GetFilesystem(std::string alias)
    {
        if (!StringUtils::EndsWith(alias, "/")) {
            alias += "/";
        }

        auto it = m_FileSystem.find(alias);
        if (it != m_FileSystem.end()) {
            return it->second;
        }
        
        return nullptr;
    }
    
    /*
     * Iterate over all registered filesystems and find first ocurrences of file
     */
    IFilePtr OpenFile(const FileInfo& filePath, IFile::FileMode mode)
    {
        for (const auto& alias : m_SortedAlias) {
			if (StringUtils::StartsWith(filePath.AbsolutePath(), alias.alias)) {
                // Strip alias from file path
                std::string relativePath = filePath.AbsolutePath().substr(alias.alias.length());
                FileInfo realPath(alias.filesystem->BasePath(), relativePath, false);

                // We open file we real path..
				IFilePtr file = alias.filesystem->OpenFile(realPath, static_cast<int>(mode));
				if (file) {
                    // ..but store with aliased path to close file later
					m_OpenedFiles[filePath.AbsolutePath()] = alias.filesystem;
					return file;
				}
			}
		}
        
        return nullptr;
    }
    
    /*
     * Close opened file if it was opened via OpenFirstFile(..)
     */
    void CloseFile(IFilePtr file)
    {
        auto it = m_OpenedFiles.find(file->GetFileInfo().AbsolutePath());
        if (it != m_OpenedFiles.end()) {
            it->second->CloseFile(file);
            m_OpenedFiles.erase(it);
        }
    }
    
private:
    struct SSortedAlias
    {
        std::string alias;
        IFileSystemPtr filesystem;
        
        SSortedAlias(const std::string& a,
                     IFileSystemPtr fs)
        : alias(a)
        , filesystem(fs)
        {}
    };
    typedef std::list<SSortedAlias> TSortedAliasList;

    TFileSystemMap m_FileSystem;
    TSortedAliasList m_SortedAlias;
    std::unordered_map<std::string, IFileSystemPtr> m_OpenedFiles;
};
    
}; // namespace vfspp

#endif // VIRTUALFILESYSTEM_H
