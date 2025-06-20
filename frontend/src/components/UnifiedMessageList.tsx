import React, { useState, useEffect } from 'react';
import { 
  Mail, 
  MessageCircle, 
  Star, 
  Trash2, 
  Reply, 
  MoreVertical,
  Clock,
  User,
  Search,
  Filter
} from 'lucide-react';

interface UnifiedMessage {
  id: string;
  accountId: string;
  sender: string;
  recipient: string;
  subject: string;
  content: string;
  messageType: 'email' | 'message' | 'notification';
  timestamp: string;
  isRead: boolean;
  isImportant: boolean;
  attachments: string[];
  metadata: Record<string, string>;
  accountProvider: string;
}

interface UnifiedMessageListProps {
  messages: UnifiedMessage[];
  onMessageSelect: (message: UnifiedMessage) => void;
  onMarkAsRead: (messageId: string) => void;
  onMarkAsImportant: (messageId: string) => void;
  onDeleteMessage: (messageId: string) => void;
  onReplyToMessage: (messageId: string) => void;
}

const UnifiedMessageList: React.FC<UnifiedMessageListProps> = ({
  messages,
  onMessageSelect,
  onMarkAsRead,
  onMarkAsImportant,
  onDeleteMessage,
  onReplyToMessage
}) => {
  const [filteredMessages, setFilteredMessages] = useState<UnifiedMessage[]>(messages);
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedFilter, setSelectedFilter] = useState<'all' | 'unread' | 'important' | 'emails' | 'messages'>('all');
  const [selectedMessage, setSelectedMessage] = useState<string | null>(null);

  useEffect(() => {
    let filtered = messages;

    // Apply search filter
    if (searchQuery) {
      filtered = filtered.filter(message =>
        message.subject.toLowerCase().includes(searchQuery.toLowerCase()) ||
        message.content.toLowerCase().includes(searchQuery.toLowerCase()) ||
        message.sender.toLowerCase().includes(searchQuery.toLowerCase())
      );
    }

    // Apply type filter
    switch (selectedFilter) {
      case 'unread':
        filtered = filtered.filter(message => !message.isRead);
        break;
      case 'important':
        filtered = filtered.filter(message => message.isImportant);
        break;
      case 'emails':
        filtered = filtered.filter(message => message.messageType === 'email');
        break;
      case 'messages':
        filtered = filtered.filter(message => message.messageType === 'message');
        break;
      default:
        break;
    }

    setFilteredMessages(filtered);
  }, [messages, searchQuery, selectedFilter]);

  const getProviderIcon = (provider: string) => {
    switch (provider.toLowerCase()) {
      case 'gmail':
        return <Mail className="w-4 h-4 text-red-500" />;
      case 'outlook':
        return <Mail className="w-4 h-4 text-blue-500" />;
      case 'whatsapp':
        return <MessageCircle className="w-4 h-4 text-green-500" />;
      case 'telegram':
        return <MessageCircle className="w-4 h-4 text-blue-400" />;
      default:
        return <MessageCircle className="w-4 h-4 text-cockpit-400" />;
    }
  };

  const getProviderColor = (provider: string) => {
    switch (provider.toLowerCase()) {
      case 'gmail':
        return 'border-l-red-500';
      case 'outlook':
        return 'border-l-blue-500';
      case 'whatsapp':
        return 'border-l-green-500';
      case 'telegram':
        return 'border-l-blue-400';
      default:
        return 'border-l-cockpit-500';
    }
  };

  const formatTimestamp = (timestamp: string) => {
    const date = new Date(timestamp);
    const now = new Date();
    const diffInHours = (now.getTime() - date.getTime()) / (1000 * 60 * 60);

    if (diffInHours < 1) {
      return 'Just now';
    } else if (diffInHours < 24) {
      return `${Math.floor(diffInHours)}h ago`;
    } else if (diffInHours < 168) { // 7 days
      return `${Math.floor(diffInHours / 24)}d ago`;
    } else {
      return date.toLocaleDateString();
    }
  };

  const handleMessageClick = (message: UnifiedMessage) => {
    setSelectedMessage(message.id);
    onMessageSelect(message);
    if (!message.isRead) {
      onMarkAsRead(message.id);
    }
  };

  return (
    <div className="h-full flex flex-col bg-dark-950">
      {/* Header */}
      <div className="p-4 border-b border-dark-700">
        <div className="flex items-center justify-between mb-4">
          <h2 className="text-xl font-semibold text-white">Unified Messages</h2>
          <div className="flex items-center space-x-2">
            <span className="text-sm text-dark-400">
              {filteredMessages.length} messages
            </span>
          </div>
        </div>

        {/* Search and Filters */}
        <div className="flex items-center space-x-4">
          <div className="flex-1 relative">
            <Search className="absolute left-3 top-1/2 transform -translate-y-1/2 w-4 h-4 text-dark-400" />
            <input
              type="text"
              placeholder="Search messages..."
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="w-full pl-10 pr-4 py-2 bg-dark-800 border border-dark-600 rounded-lg text-white placeholder-dark-400 focus:outline-none focus:border-cockpit-500"
            />
          </div>
          
          <div className="flex items-center space-x-2">
            <Filter className="w-4 h-4 text-dark-400" />
            <select
              value={selectedFilter}
              onChange={(e) => setSelectedFilter(e.target.value as any)}
              className="bg-dark-800 border border-dark-600 rounded-lg px-3 py-2 text-white focus:outline-none focus:border-cockpit-500"
            >
              <option value="all">All Messages</option>
              <option value="unread">Unread</option>
              <option value="important">Important</option>
              <option value="emails">Emails</option>
              <option value="messages">Messages</option>
            </select>
          </div>
        </div>
      </div>

      {/* Message List */}
      <div className="flex-1 overflow-y-auto">
        {filteredMessages.length === 0 ? (
          <div className="flex items-center justify-center h-full">
            <div className="text-center">
              <MessageCircle className="w-16 h-16 text-dark-600 mx-auto mb-4" />
              <p className="text-dark-400 text-lg">No messages found</p>
              <p className="text-dark-500 text-sm mt-2">
                {searchQuery ? 'Try adjusting your search terms' : 'Connect your accounts to get started'}
              </p>
            </div>
          </div>
        ) : (
          <div className="space-y-1 p-2">
            {filteredMessages.map((message) => (
              <div
                key={message.id}
                className={`relative p-4 rounded-lg cursor-pointer transition-all duration-200 ${
                  selectedMessage === message.id
                    ? 'bg-cockpit-900 border border-cockpit-500'
                    : 'bg-dark-800 hover:bg-dark-700 border border-transparent'
                } ${!message.isRead ? 'border-l-4 ' + getProviderColor(message.accountProvider) : ''}`}
                onClick={() => handleMessageClick(message)}
              >
                <div className="flex items-start justify-between">
                  <div className="flex items-start space-x-3 flex-1 min-w-0">
                    <div className="flex-shrink-0 mt-1">
                      {getProviderIcon(message.accountProvider)}
                    </div>
                    
                    <div className="flex-1 min-w-0">
                      <div className="flex items-center space-x-2 mb-1">
                        <h3 className={`font-medium truncate ${
                          !message.isRead ? 'text-white' : 'text-dark-300'
                        }`}>
                          {message.subject}
                        </h3>
                        {message.isImportant && (
                          <Star className="w-4 h-4 text-yellow-500 fill-current" />
                        )}
                        {message.attachments.length > 0 && (
                          <span className="text-xs bg-dark-600 text-dark-300 px-2 py-1 rounded">
                            📎 {message.attachments.length}
                          </span>
                        )}
                      </div>
                      
                      <div className="flex items-center space-x-2 text-sm text-dark-400 mb-2">
                        <User className="w-3 h-3" />
                        <span className="truncate">{message.sender}</span>
                        <span>•</span>
                        <span className="text-xs">{message.accountProvider}</span>
                        <span>•</span>
                        <Clock className="w-3 h-3" />
                        <span>{formatTimestamp(message.timestamp)}</span>
                      </div>
                      
                      <p className="text-sm text-dark-400 line-clamp-2">
                        {message.content}
                      </p>
                    </div>
                  </div>
                  
                  <div className="flex items-center space-x-1 ml-2">
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        onMarkAsImportant(message.id);
                      }}
                      className={`p-1 rounded hover:bg-dark-600 transition-colors ${
                        message.isImportant ? 'text-yellow-500' : 'text-dark-400 hover:text-yellow-500'
                      }`}
                    >
                      <Star className="w-4 h-4" />
                    </button>
                    
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        onReplyToMessage(message.id);
                      }}
                      className="p-1 text-dark-400 hover:text-cockpit-400 hover:bg-dark-600 rounded transition-colors"
                    >
                      <Reply className="w-4 h-4" />
                    </button>
                    
                    <button
                      onClick={(e) => {
                        e.stopPropagation();
                        onDeleteMessage(message.id);
                      }}
                      className="p-1 text-dark-400 hover:text-red-400 hover:bg-dark-600 rounded transition-colors"
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
    </div>
  );
};

export default UnifiedMessageList; 