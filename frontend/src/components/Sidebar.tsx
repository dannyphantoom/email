import React, { useState, useEffect, useCallback } from 'react';
import { Search, Plus, Users, MessageCircle } from 'lucide-react';
import { toast } from 'react-hot-toast';
import Modal from './Modal';

interface Chat {
  id: string;
  name: string;
  lastMessage: string;
  timestamp: string;
  unreadCount: number;
  isGroup: boolean;
  otherUserId?: number;
  groupId?: number;
}

interface SidebarProps {
  onChatSelect: (chatId: string) => void;
  selectedChat: string | null;
}

const Sidebar: React.FC<SidebarProps> = ({ onChatSelect, selectedChat }) => {
  const [searchQuery, setSearchQuery] = useState('');
  const [activeTab, setActiveTab] = useState<'chats' | 'groups'>('chats');
  const [chats, setChats] = useState<Chat[]>([]);
  const [groups, setGroups] = useState<Chat[]>([]);
  const [isLoading, setIsLoading] = useState(false);
  const [isGroupModalOpen, setGroupModalOpen] = useState(false);
  const [isNewChatModalOpen, setNewChatModalOpen] = useState(false);
  const [groupName, setGroupName] = useState('');
  const [groupDescription, setGroupDescription] = useState('');
  const [isCreatingGroup, setIsCreatingGroup] = useState(false);
  const [newChatUser, setNewChatUser] = useState('');
  const [isStartingChat, setIsStartingChat] = useState(false);

  const fetchChatSessions = useCallback(async () => {
    setIsLoading(true);
    try {
      const response = await fetch('/api/chat-sessions', {
        headers: {
          'Authorization': `Bearer ${localStorage.getItem('token')}`
        }
      });
      
      if (response.ok) {
        const data = await response.json();
        const chatSessions = data.data.map((session: any) => ({
          id: `chat-${session.id}`,
          name: session.other_user_name || 'Unknown User',
          lastMessage: session.last_message || 'No messages yet',
          timestamp: new Date(session.updated_at).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
          unreadCount: session.unread_count || 0,
          isGroup: false,
          otherUserId: session.other_user_id
        }));
        setChats(chatSessions);
      } else {
        toast.error('Failed to load chat sessions');
      }
    } catch (error) {
      console.error('Failed to fetch chat sessions:', error);
      toast.error('Failed to load chat sessions');
    } finally {
      setIsLoading(false);
    }
  }, []);

  const fetchGroups = useCallback(async () => {
    try {
      const response = await fetch('/api/groups', {
        headers: {
          'Authorization': `Bearer ${localStorage.getItem('token')}`
        }
      });
      
      if (response.ok) {
        const data = await response.json();
        const groupChats = data.data.map((group: any) => ({
          id: `group-${group.id}`,
          name: group.name,
          lastMessage: 'Group chat',
          timestamp: new Date(group.created_at).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
          unreadCount: 0,
          isGroup: true,
          groupId: group.id
        }));
        setGroups(groupChats);
      } else {
        toast.error('Failed to load groups');
      }
    } catch (error) {
      console.error('Failed to fetch groups:', error);
      toast.error('Failed to load groups');
    }
  }, []);

  // Fetch chat sessions and groups
  useEffect(() => {
    fetchChatSessions();
    fetchGroups();
  }, [fetchChatSessions, fetchGroups]);

  const handleCreateGroup = async () => {
    setGroupModalOpen(true);
  };

  const submitCreateGroup = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!groupName.trim()) return;
    setIsCreatingGroup(true);
    try {
      const response = await fetch('/api/groups', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${localStorage.getItem('token')}`
        },
        body: JSON.stringify({ name: groupName, description: groupDescription })
      });
      if (response.ok) {
        toast.success('Group created successfully');
        fetchGroups();
        setGroupModalOpen(false);
        setGroupName('');
        setGroupDescription('');
      } else {
        toast.error('Failed to create group');
      }
    } catch (error) {
      console.error('Failed to create group:', error);
      toast.error('Failed to create group');
    } finally {
      setIsCreatingGroup(false);
    }
  };

  const handleNewConversation = () => {
    setNewChatModalOpen(true);
  };

  const submitNewConversation = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!newChatUser.trim()) return;
    setIsStartingChat(true);
    // TODO: Implement user search and chat session creation
    toast.success('New conversation started (placeholder)');
    setNewChatModalOpen(false);
    setNewChatUser('');
    setIsStartingChat(false);
  };

  const getFilteredChats = () => {
    const currentChats = activeTab === 'chats' ? chats : groups;
    return currentChats.filter(chat =>
      chat.name.toLowerCase().includes(searchQuery.toLowerCase())
    );
  };

  const filteredChats = getFilteredChats();

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
        {isLoading ? (
          <div className="p-8 text-center">
            <div className="animate-spin rounded-full h-8 w-8 border-b-2 border-cockpit-400 mx-auto"></div>
            <p className="text-dark-400 mt-2">Loading...</p>
          </div>
        ) : filteredChats.length === 0 ? (
          <div className="p-8 text-center">
            <MessageCircle className="w-12 h-12 text-dark-600 mx-auto mb-4" />
            <p className="text-dark-400">
              {activeTab === 'chats' ? 'No conversations yet' : 'No groups yet'}
            </p>
            {activeTab === 'groups' && (
              <button
                onClick={handleCreateGroup}
                className="btn-primary mt-4 text-sm"
              >
                Create Group
              </button>
            )}
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
        {activeTab === 'groups' ? (
          <button 
            onClick={handleCreateGroup}
            className="btn-primary w-full flex items-center justify-center space-x-2"
          >
            <Plus className="w-4 h-4" />
            <span>Create Group</span>
          </button>
        ) : (
          <button
            onClick={handleNewConversation}
            className="btn-primary w-full flex items-center justify-center space-x-2"
          >
            <Plus className="w-4 h-4" />
            <span>New Conversation</span>
          </button>
        )}
      </div>
      {/* Group Creation Modal */}
      <Modal isOpen={isGroupModalOpen} onClose={() => setGroupModalOpen(false)} title="Create Group">
        <form onSubmit={submitCreateGroup} className="space-y-4">
          <div>
            <label className="block text-sm text-dark-300 mb-1">Group Name</label>
            <input
              type="text"
              value={groupName}
              onChange={e => setGroupName(e.target.value)}
              className="input-field w-full"
              required
              maxLength={50}
              placeholder="Enter group name"
            />
          </div>
          <div>
            <label className="block text-sm text-dark-300 mb-1">Description</label>
            <textarea
              value={groupDescription}
              onChange={e => setGroupDescription(e.target.value)}
              className="input-field w-full"
              maxLength={200}
              placeholder="Enter group description (optional)"
            />
          </div>
          <div className="flex justify-end space-x-2">
            <button type="button" className="btn-secondary" onClick={() => setGroupModalOpen(false)}>
              Cancel
            </button>
            <button type="submit" className="btn-primary" disabled={isCreatingGroup}>
              {isCreatingGroup ? 'Creating...' : 'Create'}
            </button>
          </div>
        </form>
      </Modal>
      {/* New Conversation Modal */}
      <Modal isOpen={isNewChatModalOpen} onClose={() => setNewChatModalOpen(false)} title="Start New Conversation">
        <form onSubmit={submitNewConversation} className="space-y-4">
          <div>
            <label className="block text-sm text-dark-300 mb-1">Username or Email</label>
            <input
              type="text"
              value={newChatUser}
              onChange={e => setNewChatUser(e.target.value)}
              className="input-field w-full"
              required
              placeholder="Enter username or email"
            />
          </div>
          <div className="flex justify-end space-x-2">
            <button type="button" className="btn-secondary" onClick={() => setNewChatModalOpen(false)}>
              Cancel
            </button>
            <button type="submit" className="btn-primary" disabled={isStartingChat}>
              {isStartingChat ? 'Starting...' : 'Start'}
            </button>
          </div>
        </form>
      </Modal>
    </div>
  );
};

export default Sidebar; 