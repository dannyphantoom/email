import React, { useState, useEffect, useRef } from 'react';
import { Routes, Route, useNavigate } from 'react-router-dom';
import { LogOut, MessageCircle, Users, Settings, Plus, Search, Inbox, Download } from 'lucide-react';
import { useAuthStore } from '../stores/authStore';
import ChatWindow from '../components/ChatWindow';
import Sidebar from '../components/Sidebar';
import UserProfileModal from '../components/UserProfileModal';
import GroupInvitations from '../components/GroupInvitations';
import { toast } from 'react-hot-toast';

const Messenger: React.FC = () => {
  const { user, logout, token } = useAuthStore();
  const navigate = useNavigate();
  const [selectedChat, setSelectedChat] = useState<string | null>(null);
  const [isUserProfileOpen, setIsUserProfileOpen] = useState(false);
  const sidebarRef = useRef<any>(null);

  const handleLogout = () => {
    logout();
    toast.success('Logged out successfully');
    navigate('/login');
  };

  if (!user) {
    return null;
  }

  return (
    <div className="h-screen flex bg-dark-950">
      {/* Sidebar */}
      <div className="w-80 bg-dark-900 border-r border-dark-700 flex flex-col">
        {/* Header */}
        <div className="p-4 border-b border-dark-700 flex-shrink-0">
          <div className="flex items-center justify-between">
            <div className="flex items-center space-x-3 min-w-0 flex-1">
              <div className="w-10 h-10 bg-gradient-to-r from-cockpit-600 to-purple-600 rounded-lg flex items-center justify-center flex-shrink-0">
                <MessageCircle className="w-6 h-6 text-white" />
              </div>
              <div className="min-w-0 flex-1">
                <h1 className="text-lg font-semibold text-gradient truncate">Cockpit</h1>
                <button
                  onClick={() => setIsUserProfileOpen(true)}
                  className="text-xs text-dark-400 truncate hover:text-white transition-colors text-left w-full"
                  title="Click to view profile"
                >
                  {user.username}@cockpit.com
                </button>
              </div>
            </div>
            <div className="flex items-center space-x-1 ml-3 flex-shrink-0">
              <button
                onClick={() => navigate('/inbox')}
                className="p-2 text-dark-400 hover:text-white hover:bg-dark-800 rounded-lg transition-colors"
                title="Unified Inbox"
              >
                <Inbox className="w-4 h-4" />
              </button>
              <button
                onClick={() => navigate('/backup')}
                className="p-2 text-dark-400 hover:text-white hover:bg-dark-800 rounded-lg transition-colors"
                title="Backup Manager"
              >
                <Download className="w-4 h-4" />
              </button>
              <button
                onClick={() => navigate('/integration')}
                className="p-2 text-dark-400 hover:text-white hover:bg-dark-800 rounded-lg transition-colors"
                title="Account Integration"
              >
                <Settings className="w-4 h-4" />
              </button>
              <button
                onClick={handleLogout}
                className="p-2 text-dark-400 hover:text-white hover:bg-dark-800 rounded-lg transition-colors"
                title="Logout"
              >
                <LogOut className="w-4 h-4" />
              </button>
            </div>
          </div>
        </div>

        {/* Sidebar Content */}
        <Sidebar ref={sidebarRef} onChatSelect={setSelectedChat} selectedChat={selectedChat} />
      </div>

      {/* Main Chat Area */}
      <div className="flex-1 flex flex-col">
        {selectedChat ? (
          <ChatWindow chatId={selectedChat} />
        ) : (
          <div className="flex-1 flex items-center justify-center">
            <div className="text-center">
              <MessageCircle className="w-16 h-16 text-dark-600 mx-auto mb-4" />
              <h2 className="text-xl font-semibold text-white mb-2">Welcome to Cockpit</h2>
              <p className="text-dark-400 mb-6">Select a conversation or start a new one to begin messaging</p>
              <div className="flex items-center justify-center space-x-4">
                <button
                  onClick={() => navigate('/inbox')}
                  className="btn-primary flex items-center space-x-2"
                >
                  <Inbox className="w-4 h-4" />
                  <span>View Unified Inbox</span>
                </button>
                <button
                  onClick={() => navigate('/backup')}
                  className="btn-secondary flex items-center space-x-2"
                >
                  <Download className="w-4 h-4" />
                  <span>Backup Manager</span>
                </button>
              </div>
            </div>
          </div>
        )}
      </div>

      {/* User Profile Modal */}
      <UserProfileModal 
        isOpen={isUserProfileOpen} 
        onClose={() => setIsUserProfileOpen(false)} 
      />

      {/* Group Invitations Popup */}
      {token && (
        <GroupInvitations 
          token={token} 
          onAction={() => {
            // Refresh the group list in the sidebar
            if (sidebarRef.current && sidebarRef.current.refreshGroups) {
              sidebarRef.current.refreshGroups();
            }
          }} 
        />
      )}
    </div>
  );
};

export default Messenger; 