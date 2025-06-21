import React, { useState, useEffect } from 'react';
import { Download, Upload, Trash2, Clock, FileText } from 'lucide-react';
import { toast } from 'react-hot-toast';
import { useAuthStore } from '../stores/authStore';

interface Backup {
  id: number;
  name: string;
  description: string;
  created_at: string;
  size: number;
}

const BackupManager: React.FC = () => {
  const { token } = useAuthStore();
  const [backups, setBackups] = useState<Backup[]>([]);
  const [isLoading, setIsLoading] = useState(false);
  const [selectedBackup, setSelectedBackup] = useState<Backup | null>(null);

  useEffect(() => {
    fetchBackups();
  }, []);

  const fetchBackups = async () => {
    setIsLoading(true);
    try {
      const response = await fetch('/api/backup', {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });

      if (response.ok) {
        const data = await response.json();
        setBackups(data.data);
      } else {
        toast.error('Failed to load backups');
      }
    } catch (error) {
      console.error('Failed to fetch backups:', error);
      toast.error('Failed to load backups');
    } finally {
      setIsLoading(false);
    }
  };

  const handleCreateBackup = async () => {
    const name = prompt('Enter backup name:');
    if (!name) return;

    const description = prompt('Enter backup description (optional):') || '';

    try {
      const response = await fetch('/api/backup', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`
        },
        body: JSON.stringify({
          name,
          description,
          data: JSON.stringify({
            timestamp: new Date().toISOString(),
            type: 'manual_backup'
          })
        })
      });

      if (response.ok) {
        toast.success('Backup created successfully');
        fetchBackups();
      } else {
        toast.error('Failed to create backup');
      }
    } catch (error) {
      console.error('Failed to create backup:', error);
      toast.error('Failed to create backup');
    }
  };

  const handleRestoreBackup = async (backup: Backup) => {
    if (!confirm(`Are you sure you want to restore "${backup.name}"? This will overwrite current data.`)) {
      return;
    }

    try {
      const response = await fetch(`/api/backup/${backup.id}/restore`, {
        method: 'POST',
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });

      if (response.ok) {
        toast.success('Backup restored successfully');
      } else {
        toast.error('Failed to restore backup');
      }
    } catch (error) {
      console.error('Failed to restore backup:', error);
      toast.error('Failed to restore backup');
    }
  };

  const handleDeleteBackup = async (backup: Backup) => {
    if (!confirm(`Are you sure you want to delete "${backup.name}"?`)) {
      return;
    }

    try {
      const response = await fetch(`/api/backup/${backup.id}`, {
        method: 'DELETE',
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });

      if (response.ok) {
        toast.success('Backup deleted successfully');
        fetchBackups();
      } else {
        toast.error('Failed to delete backup');
      }
    } catch (error) {
      console.error('Failed to delete backup:', error);
      toast.error('Failed to delete backup');
    }
  };

  const handleDownloadBackup = async (backup: Backup) => {
    try {
      const response = await fetch(`/api/backup/${backup.id}`, {
        headers: {
          'Authorization': `Bearer ${token}`
        }
      });

      if (response.ok) {
        const data = await response.json();
        const backupData = data.data;
        
        // Create and download file
        const blob = new Blob([backupData.data], { type: 'application/json' });
        const url = window.URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `${backup.name}.json`;
        document.body.appendChild(a);
        a.click();
        window.URL.revokeObjectURL(url);
        document.body.removeChild(a);
        
        toast.success('Backup downloaded successfully');
      } else {
        toast.error('Failed to download backup');
      }
    } catch (error) {
      console.error('Failed to download backup:', error);
      toast.error('Failed to download backup');
    }
  };

  const formatFileSize = (bytes: number) => {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
  };

  const formatDate = (dateString: string) => {
    return new Date(dateString).toLocaleString();
  };

  return (
    <div className="min-h-screen bg-dark-950 text-white">
      <div className="container mx-auto px-4 py-8">
        <div className="max-w-4xl mx-auto">
          {/* Header */}
          <div className="flex items-center justify-between mb-8">
            <div>
              <h1 className="text-3xl font-bold text-gradient mb-2">Backup Manager</h1>
              <p className="text-dark-400">Manage your chat backups and restore conversations</p>
            </div>
            <button
              onClick={handleCreateBackup}
              className="btn-primary flex items-center space-x-2"
            >
              <Upload className="w-4 h-4" />
              <span>Create Backup</span>
            </button>
          </div>

          {/* Backup List */}
          <div className="bg-dark-900 rounded-lg border border-dark-700">
            <div className="p-6 border-b border-dark-700">
              <h2 className="text-xl font-semibold">Your Backups</h2>
            </div>

            {isLoading ? (
              <div className="p-8 text-center">
                <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-cockpit-400 mx-auto"></div>
                <p className="text-dark-400 mt-2">Loading backups...</p>
              </div>
            ) : backups.length === 0 ? (
              <div className="p-8 text-center">
                <FileText className="w-16 h-16 text-dark-600 mx-auto mb-4" />
                <h3 className="text-lg font-medium text-white mb-2">No backups yet</h3>
                <p className="text-dark-400 mb-4">Create your first backup to get started</p>
                <button
                  onClick={handleCreateBackup}
                  className="btn-primary"
                >
                  Create First Backup
                </button>
              </div>
            ) : (
              <div className="divide-y divide-dark-700">
                {backups.map((backup) => (
                  <div key={backup.id} className="p-6 hover:bg-dark-800 transition-colors">
                    <div className="flex items-center justify-between">
                      <div className="flex-1">
                        <div className="flex items-center space-x-3 mb-2">
                          <FileText className="w-5 h-5 text-cockpit-400" />
                          <h3 className="text-lg font-medium text-white">{backup.name}</h3>
                        </div>
                        {backup.description && (
                          <p className="text-dark-400 text-sm mb-2">{backup.description}</p>
                        )}
                        <div className="flex items-center space-x-4 text-xs text-dark-500">
                          <div className="flex items-center space-x-1">
                            <Clock className="w-3 h-3" />
                            <span>{formatDate(backup.created_at)}</span>
                          </div>
                          <span>{formatFileSize(backup.size)}</span>
                        </div>
                      </div>
                      <div className="flex items-center space-x-2">
                        <button
                          onClick={() => handleDownloadBackup(backup)}
                          className="p-2 text-dark-400 hover:text-white hover:bg-dark-700 rounded-lg transition-colors"
                          title="Download Backup"
                        >
                          <Download className="w-4 h-4" />
                        </button>
                        <button
                          onClick={() => handleRestoreBackup(backup)}
                          className="p-2 text-dark-400 hover:text-cockpit-400 hover:bg-dark-700 rounded-lg transition-colors"
                          title="Restore Backup"
                        >
                          <Upload className="w-4 h-4" />
                        </button>
                        <button
                          onClick={() => handleDeleteBackup(backup)}
                          className="p-2 text-dark-400 hover:text-red-400 hover:bg-dark-700 rounded-lg transition-colors"
                          title="Delete Backup"
                        >
                          <Trash2 className="w-4 h-4" />
                        </button>
                      </div>
                    </div>
                  </div>
                ))}
              </div>
            )}
          </div>

          {/* Info Section */}
          <div className="mt-8 bg-dark-900 rounded-lg border border-dark-700 p-6">
            <h3 className="text-lg font-semibold text-white mb-4">About Backups</h3>
            <div className="grid md:grid-cols-2 gap-6">
              <div>
                <h4 className="font-medium text-white mb-2">What's included:</h4>
                <ul className="text-sm text-dark-400 space-y-1">
                  <li>• All chat messages and conversations</li>
                  <li>• Group chat data and member information</li>
                  <li>• Message timestamps and metadata</li>
                  <li>• User preferences and settings</li>
                </ul>
              </div>
              <div>
                <h4 className="font-medium text-white mb-2">Backup features:</h4>
                <ul className="text-sm text-dark-400 space-y-1">
                  <li>• Automatic encryption for security</li>
                  <li>• Download backups as JSON files</li>
                  <li>• Restore to any point in time</li>
                  <li>• Cross-device compatibility</li>
                </ul>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default BackupManager; 