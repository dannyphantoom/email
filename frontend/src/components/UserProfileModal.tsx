import React, { useState } from 'react';
import { Copy, Mail, User, Shield, Share2, Download, Edit3, Check } from 'lucide-react';
import { toast } from 'react-hot-toast';
import Modal from './Modal';
import { useAuthStore } from '../stores/authStore';

interface UserProfileModalProps {
  isOpen: boolean;
  onClose: () => void;
}

const UserProfileModal: React.FC<UserProfileModalProps> = ({ isOpen, onClose }) => {
  const { user, updateUser } = useAuthStore();
  const [backupEmail, setBackupEmail] = useState(user?.backupEmail || '');
  const [isEditingBackupEmail, setIsEditingBackupEmail] = useState(false);
  const [isSaving, setIsSaving] = useState(false);

  if (!user) return null;

  const shareLink = `${window.location.origin}/invite/${user.id}`;
  const userId = user.id.toString().padStart(8, '0');

  const copyToClipboard = async (text: string, label: string) => {
    try {
      await navigator.clipboard.writeText(text);
      toast.success(`${label} copied to clipboard`);
    } catch (error) {
      toast.error('Failed to copy to clipboard');
    }
  };

  const handleSaveBackupEmail = async () => {
    if (!backupEmail.trim()) {
      toast.error('Backup email cannot be empty');
      return;
    }

    setIsSaving(true);
    try {
      // TODO: Implement API call to update backup email
      updateUser({ backupEmail });
      setIsEditingBackupEmail(false);
      toast.success('Backup email updated successfully');
    } catch (error) {
      toast.error('Failed to update backup email');
    } finally {
      setIsSaving(false);
    }
  };

  const generateShareLink = () => {
    // TODO: Implement API call to generate a unique share link
    return shareLink;
  };

  return (
    <Modal isOpen={isOpen} onClose={onClose} title="User Profile" width="max-w-lg">
      <div className="space-y-6">
        {/* User Avatar and Basic Info */}
        <div className="text-center">
          <div className="w-20 h-20 bg-gradient-to-r from-cockpit-600 to-purple-600 rounded-full flex items-center justify-center mx-auto mb-4">
            <span className="text-white text-2xl font-bold">
              {user.username.charAt(0).toUpperCase()}
            </span>
          </div>
          <h3 className="text-xl font-semibold text-white">{user.username}</h3>
          <p className="text-dark-400">{user.email}</p>
        </div>

        {/* User ID */}
        <div className="bg-dark-800 rounded-lg p-4">
          <div className="flex items-center justify-between">
            <div className="flex items-center space-x-3">
              <User className="w-5 h-5 text-dark-400" />
              <div>
                <p className="text-sm text-dark-300">User ID</p>
                <p className="text-white font-mono">#{userId}</p>
              </div>
            </div>
            <button
              onClick={() => copyToClipboard(userId, 'User ID')}
              className="p-2 text-dark-400 hover:text-white hover:bg-dark-700 rounded-lg transition-colors"
              title="Copy User ID"
            >
              <Copy className="w-4 h-4" />
            </button>
          </div>
        </div>

        {/* Account Sharing */}
        <div className="bg-dark-800 rounded-lg p-4">
          <div className="flex items-center justify-between mb-3">
            <div className="flex items-center space-x-3">
              <Share2 className="w-5 h-5 text-dark-400" />
              <div>
                <p className="text-sm text-dark-300">Share Account</p>
                <p className="text-white text-sm">Invite others to connect with you</p>
              </div>
            </div>
          </div>
          <div className="flex items-center space-x-2">
            <input
              type="text"
              value={shareLink}
              readOnly
              className="input-field flex-1 text-sm"
            />
            <button
              onClick={() => copyToClipboard(shareLink, 'Share link')}
              className="btn-secondary px-3 py-2"
              title="Copy share link"
            >
              <Copy className="w-4 h-4" />
            </button>
          </div>
        </div>

        {/* Backup Email */}
        <div className="bg-dark-800 rounded-lg p-4">
          <div className="flex items-center justify-between mb-3">
            <div className="flex items-center space-x-3">
              <Mail className="w-5 h-5 text-dark-400" />
              <div>
                <p className="text-sm text-dark-300">Backup Email</p>
                <p className="text-white text-sm">For account recovery and notifications</p>
              </div>
            </div>
            {!isEditingBackupEmail && (
              <button
                onClick={() => setIsEditingBackupEmail(true)}
                className="p-2 text-dark-400 hover:text-white hover:bg-dark-700 rounded-lg transition-colors"
                title="Edit backup email"
              >
                <Edit3 className="w-4 h-4" />
              </button>
            )}
          </div>
          
          {isEditingBackupEmail ? (
            <div className="space-y-3">
              <input
                type="email"
                value={backupEmail}
                onChange={(e) => setBackupEmail(e.target.value)}
                className="input-field w-full"
                placeholder="Enter backup email"
              />
              <div className="flex space-x-2">
                <button
                  onClick={handleSaveBackupEmail}
                  disabled={isSaving}
                  className="btn-primary flex-1 flex items-center justify-center space-x-2"
                >
                  {isSaving ? (
                    <>
                      <div className="w-4 h-4 border-2 border-white border-t-transparent rounded-full animate-spin"></div>
                      <span>Saving...</span>
                    </>
                  ) : (
                    <>
                      <Check className="w-4 h-4" />
                      <span>Save</span>
                    </>
                  )}
                </button>
                <button
                  onClick={() => {
                    setIsEditingBackupEmail(false);
                    setBackupEmail(user?.backupEmail || '');
                  }}
                  className="btn-secondary flex-1"
                >
                  Cancel
                </button>
              </div>
            </div>
          ) : (
            <p className="text-white">
              {backupEmail || 'No backup email set'}
            </p>
          )}
        </div>

        {/* Account Security */}
        <div className="bg-dark-800 rounded-lg p-4">
          <div className="flex items-center space-x-3">
            <Shield className="w-5 h-5 text-dark-400" />
            <div>
              <p className="text-sm text-dark-300">Account Security</p>
              <p className="text-white text-sm">Member since {new Date(user.createdAt).toLocaleDateString()}</p>
            </div>
          </div>
        </div>

        {/* Export Data */}
        <div className="bg-dark-800 rounded-lg p-4">
          <div className="flex items-center justify-between">
            <div className="flex items-center space-x-3">
              <Download className="w-5 h-5 text-dark-400" />
              <div>
                <p className="text-sm text-dark-300">Export Data</p>
                <p className="text-white text-sm">Download your account data</p>
              </div>
            </div>
            <button
              onClick={() => {
                // TODO: Implement data export
                toast.success('Data export started. You will receive an email when ready.');
              }}
              className="btn-secondary"
            >
              Export
            </button>
          </div>
        </div>
      </div>
    </Modal>
  );
};

export default UserProfileModal; 