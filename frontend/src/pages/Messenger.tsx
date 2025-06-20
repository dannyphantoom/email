import React, { useState, useEffect } from 'react';
import { Routes, Route, useNavigate } from 'react-router-dom';
import { LogOut, MessageCircle, Users, Settings, Plus, Search, Inbox } from 'lucide-react';
import { useAuthStore } from '../stores/authStore';
import ChatWindow from '../components/ChatWindow';
import Sidebar from '../components/Sidebar';
import { toast } from 'react-hot-toast';

const Messenger: React.FC = () => {
  const { user, logout } = useAuthStore();
  const navigate = useNavigate();
  const [selectedChat, setSelectedChat] = useState<string | null>(null);

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
                <p className="text-xs text-dark-400 truncate">{user.username}@cockpit.com</p>
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
        <Sidebar onChatSelect={setSelectedChat} selectedChat={selectedChat} />
      </div>

      {/* Main Chat Area */}
      <div className="flex-1 flex flex-col">
        {selectedChat ? (
          <ChatWindow chatId={selectedChat} />
        ) : (
          <div className="flex-1 flex items-center justify-center chat-animated-bg">
            <div className="text-center">
              <div className="w-24 h-24 bg-gradient-to-r from-cockpit-600 to-purple-600 rounded-full flex items-center justify-center mx-auto mb-6">
                <MessageCircle className="w-12 h-12 text-white" />
              </div>
              <h2 className="text-2xl font-semibold text-white mb-2">Welcome to Cockpit</h2>
              <p className="text-dark-400 mb-6">Select a conversation to start messaging</p>
              <div className="flex space-x-4 justify-center">
                <button className="btn-primary flex items-center space-x-2">
                  <Plus className="w-5 h-5" />
                  <span>New Conversation</span>
                </button>
                <button 
                  onClick={() => navigate('/inbox')}
                  className="btn-secondary flex items-center space-x-2"
                >
                  <Inbox className="w-5 h-5" />
                  <span>Unified Inbox</span>
                </button>
                <button 
                  onClick={() => navigate('/integration')}
                  className="btn-secondary flex items-center space-x-2"
                >
                  <Settings className="w-5 h-5" />
                  <span>Connect Accounts</span>
                </button>
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};

export default Messenger; 