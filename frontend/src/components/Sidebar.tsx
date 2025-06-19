import React, { useState } from 'react';
import { Search, Plus, Users, MessageCircle } from 'lucide-react';

interface Chat {
  id: string;
  name: string;
  lastMessage: string;
  timestamp: string;
  unreadCount: number;
  isGroup: boolean;
}

interface SidebarProps {
  onChatSelect: (chatId: string) => void;
  selectedChat: string | null;
}

const Sidebar: React.FC<SidebarProps> = ({ onChatSelect, selectedChat }) => {
  const [searchQuery, setSearchQuery] = useState('');
  const [activeTab, setActiveTab] = useState<'chats' | 'groups'>('chats');

  // Mock data - replace with actual data from API
  const chats: Chat[] = [
    {
      id: '1',
      name: 'john@cockpit.com',
      lastMessage: 'Hey, how are you doing?',
      timestamp: '2:30 PM',
      unreadCount: 2,
      isGroup: false,
    },
    {
      id: '2',
      name: 'sarah@cockpit.com',
      lastMessage: 'The meeting is scheduled for tomorrow',
      timestamp: '1:45 PM',
      unreadCount: 0,
      isGroup: false,
    },
    {
      id: '3',
      name: 'Project Team',
      lastMessage: 'Alice: Great work everyone!',
      timestamp: '12:20 PM',
      unreadCount: 5,
      isGroup: true,
    },
  ];

  const filteredChats = chats.filter(chat =>
    chat.name.toLowerCase().includes(searchQuery.toLowerCase())
  );

  return (
    <div className="flex-1 flex flex-col">
      {/* Search Bar */}
      <div className="p-4">
        <div className="relative">
          <Search className="absolute left-3 top-1/2 transform -translate-y-1/2 w-4 h-4 text-dark-400" />
          <input
            type="text"
            placeholder="Search conversations..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="input-field pl-10 w-full text-sm"
          />
        </div>
      </div>

      {/* Tabs */}
      <div className="flex border-b border-dark-700">
        <button
          onClick={() => setActiveTab('chats')}
          className={`flex-1 py-3 px-4 text-sm font-medium transition-colors ${
            activeTab === 'chats'
              ? 'text-cockpit-400 border-b-2 border-cockpit-400'
              : 'text-dark-400 hover:text-white'
          }`}
        >
          <MessageCircle className="w-4 h-4 inline mr-2" />
          Chats
        </button>
        <button
          onClick={() => setActiveTab('groups')}
          className={`flex-1 py-3 px-4 text-sm font-medium transition-colors ${
            activeTab === 'groups'
              ? 'text-cockpit-400 border-b-2 border-cockpit-400'
              : 'text-dark-400 hover:text-white'
          }`}
        >
          <Users className="w-4 h-4 inline mr-2" />
          Groups
        </button>
      </div>

      {/* Chat List */}
      <div className="flex-1 overflow-y-auto">
        {filteredChats.length === 0 ? (
          <div className="p-8 text-center">
            <MessageCircle className="w-12 h-12 text-dark-600 mx-auto mb-4" />
            <p className="text-dark-400">No conversations found</p>
          </div>
        ) : (
          <div className="space-y-1 p-2">
            {filteredChats.map((chat) => (
              <div
                key={chat.id}
                onClick={() => onChatSelect(chat.id)}
                className={`sidebar-item ${
                  selectedChat === chat.id ? 'sidebar-item-active' : ''
                }`}
              >
                <div className="flex-shrink-0 w-10 h-10 bg-gradient-to-r from-cockpit-600 to-purple-600 rounded-full flex items-center justify-center">
                  {chat.isGroup ? (
                    <Users className="w-5 h-5 text-white" />
                  ) : (
                    <span className="text-white font-medium text-sm">
                      {chat.name.charAt(0).toUpperCase()}
                    </span>
                  )}
                </div>
                <div className="flex-1 min-w-0">
                  <div className="flex items-center justify-between">
                    <h3 className="text-sm font-medium text-white truncate">
                      {chat.name}
                    </h3>
                    <span className="text-xs text-dark-400">{chat.timestamp}</span>
                  </div>
                  <p className="text-xs text-dark-400 truncate">{chat.lastMessage}</p>
                </div>
                {chat.unreadCount > 0 && (
                  <div className="flex-shrink-0 w-5 h-5 bg-cockpit-500 text-white text-xs rounded-full flex items-center justify-center">
                    {chat.unreadCount}
                  </div>
                )}
              </div>
            ))}
          </div>
        )}
      </div>

      {/* New Chat Button */}
      <div className="p-4 border-t border-dark-700">
        <button className="btn-primary w-full flex items-center justify-center space-x-2">
          <Plus className="w-4 h-4" />
          <span>New Conversation</span>
        </button>
      </div>
    </div>
  );
};

export default Sidebar; 